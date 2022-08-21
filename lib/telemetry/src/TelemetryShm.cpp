/*******************************************************************************
* Name: TelemetryShm.cpp
*
* Purpose: Telemetry shared memory access and control for readers and writers
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/mman.h>
#include "lib/dls/dls.h"
#include "lib/telemetry/TelemetryShm.h"

using namespace dls;

// TODO add more logging messages

// how read locking works in blocking mode for an individual packets:
// obtain lock to all nonces (one lock shared by everyone)
// cache the latest master nonce
// check our cached nonces with the nonces from shared memory (which are locked)
// if any changed, return immediately and keep the lock
// if none changed, we need to wait until they do
// release the locks
// call futex wait, blocks us until someone updates one of our packets
//      futex wait checks if someone updated since we unblocked and returns immediately so we can check nonces again
// when we wake up, lock and check nonces again from the top

// locking on all packets works similarly but we only check our cached master nonce
// against the shared master nonce rather than checking nonces for each packet

// biggest efficiency problem is all readers and writers contend for one lock
// since they all need to call futex to block, they need one word to block on and wait for changes to
// to do this for multiple packets, we would need to call futex on all the nonces and wake up at the first one that changes
// which means we need threads and to wait for the return of one of those threads, which is slow as hell
// so we share a lock instead

// when we do call futex_wait, we use a bitset so we don't get woken up for packets that aren't us
// the packet id is used in the bitset so when a packet with the same id mod 32 is written, it calls
// wake on that bitset


// locking is done with writers preference
// https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem

// P and V semaphore macros
#define P(X) \
    if(0 != sem_wait( &( (X) ) )) { \
        return FAILURE; \
    } \

#define V(X) \
    if(0 != sem_post( &( (X) ) )) { \
        return FAILURE; \
    } \

// initialize semaphore macro
#define INIT(X, V) \
    if(0 != sem_init( &( (X) ), 1, (V) )) { \
        return FAILURE; \
    } \


TelemetryShm::TelemetryShm() {
    packet_blocks = NULL;
    info_blocks = NULL;
    master_block = NULL;
    num_packets = 0;
    last_nonces = NULL;
    last_nonce = 1; // NOTE: cannot be 0, 0 indicates a signal was received
    read_mode = STANDARD_READ;
    read_locked = false;
}

TelemetryShm::~TelemetryShm() {
    // unlock all the packets we have write locks on
    for(size_t i = 0; i < num_packets; i++) {
        if(locked_packets[i]) {
            // unlock this packet
            write_unlock(i);
        }
    }

    // if we're read locked, unlock now
    if(read_locked) {
        read_unlock();
    }

    if(last_nonces) {
        free(last_nonces);
    }

    if(updated) {
        free(updated);
    }
}

RetType TelemetryShm::init(VCM* vcm) {
    num_packets = vcm->num_packets;

    // create and set last_nonces
    last_nonces = (uint32_t*)malloc(num_packets * sizeof(uint32_t));
    // memset(last_nonces, 0, num_packets * sizeof(uint32_t));
    for(size_t i = 0; i < num_packets; i++) {
        last_nonces[i] = 1;
    }

    // create and zero updated array
    updated = (bool*)malloc(num_packets * sizeof(bool));
    memset(updated, 0, num_packets * sizeof(bool));

    // create master Shm object
    // use an id guaranteed unused so we can use the same file name for all blocks
    master_block = boost::interprocess::offset_ptr<Shm>(new Shm(vcm->config_file.c_str(), 0, sizeof(shm_info_t)));

    // create Shm objects for each telemetry packet
    packet_blocks = std::make_unique<boost::interprocess::offset_ptr<Shm>[]>(num_packets);
    info_blocks = std::make_unique<boost::interprocess::offset_ptr<Shm>[]>(num_packets);
    write_locks = std::make_unique<boost::interprocess::offset_ptr<Shm>[]>(num_packets);

    // store which packets we currently have locked
    locked_packets = std::make_unique<bool[]>(num_packets);

    packet_info_t const* packet;
    for (size_t i = 0; i < num_packets; i++) {
        packet = vcm->packets[i];
        // for shmem id use (i+1)*2 for packets (always even) and (2*i)+1 for info blocks (always odd)
        // virtual locks use a shmid of -(i+1)*2 (always even and negative)
        // guarantees all blocks can use the same file but different ids to make a key
        packet_blocks.get()[i] = boost::interprocess::offset_ptr<Shm>(new Shm(vcm->config_file.c_str(), 2 * (i + 1), packet->size));


        info_blocks.get()[i] = boost::interprocess::offset_ptr<Shm>(new Shm(vcm->config_file.c_str(), (2 * i) + 1,sizeof(uint32_t))); // holds one nonce

        write_locks.get()[i] = boost::interprocess::offset_ptr<Shm>(new Shm(vcm->config_file.c_str(), -2 * (i + 1), sizeof(sem_t))); // holds a single semaphore

        // we currently hold no locks
        locked_packets[i] = false;
    }

    return SUCCESS;
}

RetType TelemetryShm::init() {
    VCM vcm; // leave this on stack to be discarded after return
    return init(&vcm);
}

RetType TelemetryShm::open() {
    for(size_t i = 0; i < num_packets; i++) {
        if (SUCCESS != packet_blocks[i]->attach()) {
            return FAILURE;
        } else if (SUCCESS != info_blocks[i]->attach()) {
            return FAILURE;
        } else if (SUCCESS != write_locks[i]->attach()) {
            return FAILURE;
        }
    }

    if(SUCCESS != master_block->attach()) {
        return FAILURE;
    }

    return SUCCESS;
}

RetType TelemetryShm::close() {
    for(size_t i = 0; i < num_packets; i++) {
        if(SUCCESS != packet_blocks[i]->detach()) {
            return FAILURE;
        }
        else if(SUCCESS != info_blocks[i]->detach()) {
            return FAILURE;
        } else if(SUCCESS != write_locks[i]->detach()) {
            return FAILURE;
        }
    }

    if(SUCCESS != master_block->detach()) {
        return FAILURE;
    }

    return SUCCESS;
}

// NOTE: does not attach!
RetType TelemetryShm::create() {
    MsgLogger logger("TelemetryShm", "create");

    for(size_t i = 0; i < num_packets; i++) {
        if(SUCCESS != packet_blocks[i]->create()) {
            logger.log_message("failed to create packet block");
            return FAILURE;
        }
        else if(SUCCESS != info_blocks[i]->create()) {
            logger.log_message("failed to create info block");
            return FAILURE;
        } else if(SUCCESS != write_locks[i]->create()) {
            logger.log_message("failed to create write lock");
            return FAILURE;
        }

        // attach first
        if(SUCCESS != info_blocks[i]->attach()) {
            logger.log_message("failed to attach to shared memory block");
            return FAILURE;
        }

        // 'info blocks' are single nonces now, initialize them to 1
        *((uint32_t*)info_blocks[i]->data) = 1;

        // we should unatach after setting the default
        // although we technically still could stay attached and be okay
        if(SUCCESS != info_blocks[i]->detach()) {
            logger.log_message("failed to detach from shared memory block");
            return FAILURE;
        }

        // now set the write lock for this packet
        if(SUCCESS != write_locks[i]->attach()) {
            logger.log_message("failed to attach to write lock shared memory block");
            return FAILURE;
        }

        INIT(*((sem_t*)(write_locks[i]->data)), 1);

        if(SUCCESS != write_locks[i]->detach()) {
            logger.log_message("failed to detach from write lock shared memory block");
            return FAILURE;
        }
    }

    if(SUCCESS != master_block->create()) {
        logger.log_message("failed to create master block");
        return FAILURE;
    }

    // need to attach in order to preset data
    if(SUCCESS != master_block->attach()) {
        logger.log_message("failed to attach to master block");
        return FAILURE;
    }

    shm_info_t* info = (shm_info_t*)master_block->data;

    // init semaphores
    INIT(info->rmutex, 1);
    INIT(info->wmutex, 1);
    INIT(info->readTry, 1);
    INIT(info->resource, 1);

    // init reader/writer counts to 0
    info->readers = 0;
    info->writers = 0;

    // start the master nonce at 1, 0 indicates a signal
    info->nonce = 1;

    // we should detach to be later attached
    // if this fails it's not the end of the world? but its still bad and shouldn't fail
    if(SUCCESS != master_block->detach()) {
        logger.log_message("failed to detach from master block");
        return FAILURE;
    }

    return SUCCESS;
}

// NOTE: must be attached already!
RetType TelemetryShm::destroy() {
    MsgLogger logger("TelemetryShm", "destroy");

    for(size_t i = 0; i < num_packets; i++) {
        if(SUCCESS != packet_blocks[i]->destroy()) {
            logger.log_message("failed to destroy packet block");
            return FAILURE;
        }
        else if(SUCCESS != info_blocks[i]->destroy()) {
            logger.log_message("failed to destroy info block");
            return FAILURE;
        } else if(SUCCESS != write_locks[i]->destroy()) {
            logger.log_message("failed to destroy write lock block");
            return FAILURE;
        }
    }

    if(SUCCESS != master_block->destroy()) {
        logger.log_message("failed to destroy master block");
        return FAILURE;
    }

    return SUCCESS;
}

RetType TelemetryShm::write(uint32_t packet_id, uint8_t* data) {
    MsgLogger logger("TelemetryShm", "write");

    if(packet_id >= num_packets) {
        logger.log_message("invalid packet id");
        return FAILURE;
    }

    if(packet_blocks == NULL || info_blocks == NULL || master_block == NULL) {
        // not open
        logger.log_message("object not open");
        return FAILURE;
    }

    // packet_id is an index
    Shm* packet = packet_blocks[packet_id].get();
    uint32_t* packet_nonce = (uint32_t*)info_blocks[packet_id]->data;
    shm_info_t* info = (shm_info_t*)master_block->data;

    if(packet == NULL || info == NULL || packet_nonce == NULL) {
        logger.log_message("shared memory block is null");
        // something isn't attached
        return FAILURE;
    }

    // enter as a writer
    P(info->wmutex);
    info->writers++;
    if(info->writers == 1) {
        P(info->readTry);
    }
    V(info->wmutex);

    P(info->resource);

    memcpy((unsigned char*)packet->data, data, packet->size);
    info->nonce++; // update the master nonce

    (*packet_nonce) = info->nonce; // update the packet nonce to equal the new master nonce

    // update our last nonces
    // we do this so if we read after a write we know we updated our packet
    // TODO is this bad? if we write and then do a read it will block
    // for now, this seems like the way to go since only mmon reads and writes and keeps it's own cache so this is preferable
    // last_nonce = info->nonce;
    // last_nonces[packet_id] = last_nonce;

    // TODO remove the above because then we never trigger on virtual values

    // wakeup anyone blocked on this packet (or any packet with an equivalen id mod 32)
    syscall(SYS_futex, &(info->nonce), FUTEX_WAKE_BITSET, INT_MAX, NULL, NULL, 1 << (packet_id % 32)); // TODO check return
    // syscall(SYS_futex, &(info->nonce), FUTEX_WAKE, INT_MAX, NULL, NULL, NULL); // TODO check return


    // exit as a writer
    V(info->resource);

    P(info->wmutex);
    info->writers--;
    if(info->writers == 0) {
        V(info->readTry);
    }
    V(info->wmutex);

    return SUCCESS;
}

RetType TelemetryShm::clear(uint32_t packet_id, uint8_t val) {
    MsgLogger logger("TelemetryShm", "clear");

    if(packet_id >= num_packets) {
        logger.log_message("invalid packet id");
        return FAILURE;
    }

    if(packet_blocks == NULL || info_blocks == NULL || master_block == NULL) {
        // not open
        logger.log_message("object not open");
        return FAILURE;
    }

    // packet_id is an index
    Shm* packet = packet_blocks[packet_id].get();
    uint32_t* packet_nonce = (uint32_t*)info_blocks[packet_id]->data;
    shm_info_t* info = (shm_info_t*)master_block->data;

    if(packet == NULL || info == NULL || packet_nonce == NULL) {
        logger.log_message("shared memory block is null");
        // something isn't attached
        return FAILURE;
    }

    // enter as a writer
    P(info->wmutex);
    info->writers++;
    if(info->writers == 1) {
        P(info->readTry);
    }
    V(info->wmutex);

    P(info->resource);

    memset((unsigned char*)packet->data, val, packet->size);
    info->nonce++; // update the master nonce

    (*packet_nonce) = info->nonce; // update the packet nonce to equal the new master nonce

    // update our last nonces
    // we do this so if we read after a write we know we updated our packet
    // TODO is this bad? if we write and then do a read it will block
    // for now, this seems like the way to go since only mmon reads and writes and keeps it's own cache so this is preferable
    // last_nonce = info->nonce;
    // last_nonces[packet_id] = last_nonce;

    // TODO remove the above because then we never trigger on virtual values

    // wakeup anyone blocked on this packet
    // technically we can only block on up to 32 packets, but we mod the packet id so that
    // some packets may have to share, the reader should check to see if their packet really updated
    syscall(SYS_futex, info->nonce, FUTEX_WAKE_BITSET, INT_MAX, NULL, NULL, 1 << (packet_id % 32));

    // exit as a writer
    V(info->resource);

    P(info->wmutex);
    info->writers--;
    if(info->writers == 0) {
        V(info->readTry);
    }
    V(info->wmutex);

    return SUCCESS;
}

RetType TelemetryShm::read_lock(std::unique_ptr<uint32_t> packet_ids, size_t num, uint32_t timeout) {
    MsgLogger logger("TelemetryShm", "read_lock(2 args)");

    if(read_locked) {
        logger.log_message("shared memory already locked");
        return FAILURE;
    }

    if(info_blocks == NULL || master_block == NULL) {
        // not open
        logger.log_message("object not open");
        return FAILURE;
    }

    shm_info_t* info = (shm_info_t*)master_block->data;

    if(info == NULL) {
        // something isn't attached
        logger.log_message("shared memory block is null");
        return FAILURE;
    }

    // zero the updated array to track what changed in this lock
    memset(updated, 0, num_packets * sizeof(bool));

    // set timeout if there is one
    // NOTE: we use an absolute value for 'timespec' NOT relative
    // see 'man futex' under FUTEX_WAIT section
    struct timespec* timespec = NULL;
    if(timeout > 0) {
        struct timespec curr_time;
        // TODO setting to CLOCK_REALTIME and ORing futex op with FUTEX_CLOCK_REALTIME doesnt seem to work...
        clock_gettime(CLOCK_MONOTONIC, &curr_time);

        uint32_t ms = curr_time.tv_sec * 1000;
        ms += (curr_time.tv_nsec / 1000000);
        ms += timeout;

        struct timespec time;
        time.tv_sec = ms / 1000;
        time.tv_nsec = (ms % 1000) * 1000000;
        timespec = &time;
    }

    // if we block, keep looping since we can't guarantee just because we were awoken our packet changed
    // this is due to having only 32 bits in the bitset but arbitrarily many packets
    // TODO in the future this loop could be avoided by setting a hard limit of 32 on the number of packets
    // then just immediately return after the futex syscall returns
    // otherwise we return at some point
    while(1) {
        // enter as a reader
        P(info->readTry);
        P(info->rmutex);
        info->readers++;
        if(info->readers == 1) {
            P(info->resource);
        }
        V(info->rmutex);
        V(info->readTry);

        // update the stored master nonce
        last_nonce = info->nonce;
        if(last_nonce == 0) {
            // we got a signal sometime before now which set the master nonce to 0
            // exit immediately
            logger.log_message("caught signal, exiting");
            return INTERRUPTED;
        }
        // if a signal happens after this point, the master nonce memory will be set to 0
        // which will not match our 'last_nonce' and futex will return immediately

        // check to see if any nonce has changed for the packets we're locking
        // if any nonce has changed we don't need to block
        uint32_t bitset = 0;
        uint32_t* nonce;
        unsigned int id;
        bool block = true; // whether or not we block
        for(size_t i = 0; i < num; i++) {
            bitset |= (1 << i);

            id = packet_ids.get()[i];

            nonce = (uint32_t*)(info_blocks[id]->data);
            if(*nonce != last_nonces[id]) {
                // we found a nonce that changed!
                // important to not just return here since we may have other stored nonces to update
                last_nonces[id] = *nonce;
                block = false;
                updated[id] = true;
            }
        }

        // in standard read mode we don't care if the packet updated
        if(!block || (read_mode == STANDARD_READ)) {
            read_locked = true;
            return SUCCESS;
        }

        // if we made it here none of our nonces changed :(
        // time to block
        read_locked = true;
        read_unlock();
        read_locked = false;

        if(read_mode == BLOCKING_READ) {
            if(-1 == syscall(SYS_futex, &info->nonce, FUTEX_WAIT_BITSET, last_nonce, timespec, NULL, bitset)) {
                if(errno == ETIMEDOUT) {
                    logger.log_message("shared memory wait timed out");

                    return TIMEOUT;
                }

                // if we get EAGAIN, it could mean that the nonce changed before we could block OR we got a signal and it was remapped and set to 0
                if(errno != EAGAIN) {
                    // else something bad
                    return FAILURE;
                }
            } // otherwise we've been woken up

            // we can check the nonce here and it's not a race condition
            // if it's zero, the memory is no longer shared so it doesn't matter
            // if it's in shared memory, it will never be zero so this doesn't matter
            if(info->nonce == 0) {
                // if this is 0, we got a signal and should exit
                logger.log_message("woke up after signal, exiting");
                return INTERRUPTED;
            }
        } else { // NONBLOCKING_READ
            return BLOCKED;
        }
    }
}

// see about updating all nonces in read_unlock?? (maybe except master nonce since it's needed for locking)
// ^^^ did the above, check if that's correct...
// I THINK IT'S WRONG then next time you call lock and it was technically updated it blocks...
// so they should probably all be updated here and only the ones being checked in the other version
RetType TelemetryShm::read_lock(uint32_t timeout) {
    MsgLogger logger("TelemetryShm", "read_lock(no args)");

    if(read_locked) {
        logger.log_message("shared memory already locked");
        return FAILURE;
    }

    if(info_blocks == NULL || master_block == NULL) {
        // not open
        logger.log_message("object not open");
        return FAILURE;
    }

    shm_info_t* info = (shm_info_t*)master_block->data;

    if(info == NULL) {
        logger.log_message("shared memory block is null");
        // something isn't attached
        return FAILURE;
    }

    // zero the updated array to track what packets changed
    memset(updated, 0, num_packets * sizeof(bool));

    // set timeout if there is one
    // NOTE: we use an absolute value for 'timespec' NOT relative
    // see 'man futex' under FUTEX_WAIT section
    struct timespec* timespec = NULL;
    if(timeout > 0) {
        struct timespec curr_time;
        // TODO setting to CLOCK_REALTIME and ORing futex op with FUTEX_CLOCK_REALTIME doesnt seem to work...
        clock_gettime(CLOCK_MONOTONIC, &curr_time);

        uint32_t ms = curr_time.tv_sec * 1000;
        ms += (curr_time.tv_nsec / 1000000);
        ms += timeout;

        struct timespec time;
        time.tv_sec = ms / 1000;
        time.tv_nsec = (ms % 1000) * 1000000;
        timespec = &time;
    }

    while(1) {
        // enter as a reader
        P(info->readTry);
        P(info->rmutex);
        info->readers++;
        if(info->readers == 1) {
            P(info->resource);
        }
        V(info->rmutex);
        V(info->readTry);

        // if reading in standard mode we never block so don't check
        if(read_mode == STANDARD_READ) {
            // update the master nonce
            last_nonce = info->nonce;

            // update all the stored packet nonces
            for(size_t i = 0; i < num_packets; i++) {
                uint32_t nonce = *((uint32_t*)(info_blocks[i]->data));

                if(last_nonces[i] != nonce) {
                    updated[i] = true;
                    last_nonces[i] = nonce;
                }
            }

            read_locked = true;
            return SUCCESS;
        }

        if(last_nonce == info->nonce) { // nothing changed, block
            read_locked = true;
            read_unlock();
            read_locked = false;

            if(read_mode == BLOCKING_READ) {
                // wait for any packet to be updated
                // we don't need to check if the nonce is 0 (indicating a signal) since if it is it will never match 'last_nonce' (which can never be 0)
                if(-1 == syscall(SYS_futex, &info->nonce, FUTEX_WAIT_BITSET, last_nonce, timespec, NULL, 0xFFFFFFFF)) {
                    if(errno == ETIMEDOUT) {
                        logger.log_message("shared memory wait timed out");

                        return TIMEOUT;
                    }

                    // if we get EAGAIN, it could mean that the nonce changed before we could block OR we got a signal and it was remapped and set to 0
                    if(errno != EAGAIN) {
                        // else something bad
                        return FAILURE;
                    }
                } // otherwise we've been woken up

                // we can check the nonce here and it's not a race condition
                // if it's zero, the memory is no longer shared so it doesn't matter
                // if it's in shared memory, it will never be zero so this doesn't matter
                if(info->nonce == 0) {
                    // if this is 0, we got a signal and should exit
                    logger.log_message("woke up after signal, exiting");
                    return INTERRUPTED;
                }
            } else {
                return BLOCKED;
            }
        } else {
            // update all the stored packet nonces
            for(size_t i = 0; i < num_packets; i++) {
                uint32_t nonce = *((uint32_t*)(info_blocks[i]->data));
                if(last_nonces[i] != nonce) {
                    updated[i] = true;
                    last_nonces[i] = nonce;
                }
            }

            // update the master nonce
            last_nonce = info->nonce;

            read_locked = true;
            return SUCCESS;
        }

    }
}

// handle a signal, should be called from a sighandler
// takes of the case where a process is blocking in 'read_lock' and gets a signal
// need to make the function return and stop blocking
// NOT useful for writers to use, the 'write' functions should be allowed to
// finish if a signal is received, and then have the process exit after
void TelemetryShm::sighandler() {
    MsgLogger logger("TelemetryShm", "sighandler");

    if(master_block == NULL) {
        // we aren't attached, just return
        return;
    }

    shm_info_t* info = (shm_info_t*)master_block->data;
    void* futex_word = &(info->nonce);

    // TODO is this bad? what if we hold a lock in the master block when we get the sighandler?
    //      then we would never release the lock since we're detached
    if(FAILURE == master_block->detach()) {
        logger.log_message("Failed to detach from shm");
        // we shouldn't do anything else, trying to stop the process failed and we'll stay running
        return;
    }

    // remap the futex word virtual address and then change the data
    // it will look like the nonce changed when the syscall restarts but it didn't
    // change in shared memory
    // we reserve the value zero for the nonce so nothing else uses, guaranteeing the syscall will always fail
    // the only case this would be a problem is if the master nonce wraps around, which will happen once every 4 years of running or so
    void* addr = mmap(futex_word, 4, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);

    if(addr != futex_word) {
        // mmap failed to map to the address we wanted, leave process running
        logger.log_message("mmap failed to map to correct address");
        return;
    }

    // set nonce to zero so futex_wait returns EAGAIN on the restart
    // NOTE: we could skip this, mmap will initialize it to 0
    *((int*)addr) = 0;
}

RetType TelemetryShm::read_unlock(bool force) {
    MsgLogger logger("TelemetryShm", "read_unlock");

    if(!read_locked && !force) {
        logger.log_message("shared memory is not locked");
        return FAILURE;
    }

    if(info_blocks == NULL || master_block == NULL) {
        // not open
        logger.log_message("object not open");
        return FAILURE;
    }

    shm_info_t* info = (shm_info_t*)master_block->data;

    if(info == NULL) {
        logger.log_message("shared memory block is null");
        // something isn't attached
        return FAILURE;
    }

    // exit as a reader
    P(info->rmutex);
    info->readers--;
    if(info->readers == 0) {
        V(info->resource);
    }
    V(info->rmutex);

    read_locked = false;
    return SUCCESS;
}

RetType TelemetryShm::write_lock(uint32_t packet_id) {
    MsgLogger logger("TelemetryShm", "write_lock");

    if(packet_id >= num_packets) {
        logger.log_message("invalid packet id");
        return FAILURE;
    }

    if(locked_packets[packet_id]) {
        logger.log_message("packet already locked");
        return FAILURE;
    }

    // NOTE: we assume that we can obtain this lock fast
    // we don't check if we caught a signal before blocking

    Shm* lock = write_locks[packet_id].get();
    P(*((sem_t*)(lock->data)));
    locked_packets[packet_id] = true;

    return SUCCESS;
}

RetType TelemetryShm::write_unlock(uint32_t packet_id) {
    MsgLogger logger("TelemetryShm", "write_unlock");

    if(packet_id >= num_packets) {
        logger.log_message("invalid packet id");
        return FAILURE;
    }

    if(!locked_packets[packet_id]) {
        logger.log_message("packet already unlocked");
        return FAILURE;
    }

    Shm* lock = write_locks[packet_id].get();
    V(*((sem_t*)(lock->data)));
    locked_packets[packet_id] = false;

    return SUCCESS;
}

uint8_t* TelemetryShm::get_buffer(uint32_t packet_id) {
    if(packet_id >= num_packets) {
        MsgLogger logger("TelemtryShm", "get_buffer");
        logger.log_message("invalid packet id");

        return NULL;
    }

    if(packet_blocks == NULL) {
        MsgLogger logger("TelemtryShm", "get_buffer");
        logger.log_message("shared memory block is null");

        return NULL;
    }

    return packet_blocks[packet_id]->data;
}

// NOTE: faster to just check the 'updated' array
RetType TelemetryShm::packet_updated(uint32_t packet_id, bool* updated) {
    MsgLogger logger("TelemetryShm", "packet_updated");

    if(!read_locked) {
        logger.log_message("not read locked");
        return FAILURE;
    }

    if(packet_id >= num_packets) {
        logger.log_message("invalid packet id");
        return FAILURE;
    }

    // get the nonce from shared memory
    uint32_t* nonce = (uint32_t*)(info_blocks[packet_id]->data);

    // compare it to our last stored nonce
    // don't need to do any locking, we're only reading and any change to the nonce will cause them to differ
    *updated = (last_nonces[packet_id] == *nonce);

    return SUCCESS;
}


RetType TelemetryShm::update_value(uint32_t packet_id, uint32_t* value) {
    MsgLogger logger("TelemetryShm", "update_value");

    if(packet_id >= num_packets) {
        logger.log_message("invalid packet id");
        return FAILURE;
    }

    // return difference between last master nonce and last packet nonce
    // the smaller the difference, the more recent the packet
    // a value of 0 indicates the packet was updated before the last call to 'read_lock'
    *value = last_nonce - last_nonces[packet_id];

    return SUCCESS;
}

RetType TelemetryShm::more_recent_packet(uint32_t* packet_ids, size_t num, uint32_t* recent) {
    MsgLogger logger("TelemetryShm", "more_recent_packet");

    uint32_t best_diff = UINT_MAX;
    // uint32_t master_nonce = *((uint32_t*)((shm_info_t*)master_block->data));

    unsigned int id;
    long int diff;
    for(size_t i = 0; i < num; i++) {
        id = packet_ids[i];
        if(id >= num_packets) {
            // invalid packet id
            logger.log_message("invalid packet id");
            return FAILURE;
        }

        // find the nonce with the smallest value different from the master nonce (guaranteed to change every update)
        diff = last_nonce - last_nonces[i];
        if(diff < best_diff) {
            *recent = i;
        }
    }

    return SUCCESS;
}

void TelemetryShm::set_read_mode(read_mode_t mode) {
    read_mode = mode;
}

#undef P
#undef V
#undef INIT
