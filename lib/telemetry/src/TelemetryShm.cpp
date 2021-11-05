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

// how locking works right now (for individual packet):
//  lock master block
//  check each packet nonce against last saved nonces
//      during this check save all the new nonces if any changed
//
//  if a nonce changed, return
//  if a nonce didn't change and need to block
//      unlock master block
//      futex wait bitset on master nonce
//      when wake up, check all the nonces again and repeat
//
//  why is this bad? one lock for all readers/writers even if working on different packet
//      also, overflow isn't detected when looking for packet updates

// potential other way:
//  store the master nonce locally
//      if it changes after the individual packet nonces are checked, we should recheck them
//      only want to do this if we need to sleep the process
//  check each packet nonce against last saved nonces
//      don't need to lock here since if they change while we check, that's actually good
//
//  if a nonce changed
//      lock individual locks for each packet
//      return
//
//  if a nonce didn't change
//      futex bitset on the master nonce we stored before and the master nonce
//          writers should just change this every write so it knows to wake people up
//
//  why is this bad? still no way to tell who was updated more recently, I think each nonce should just get incremented and check for overflow, resetting the others if it happens
//      locking all packets may be a bit harder actually, need to lock a lot of individual blocks
//      could keep a master lock? i have absolutely no idea how that would work


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
    force_woken = false;
}

TelemetryShm::~TelemetryShm() {
    if(packet_blocks) {
        for(size_t i = 0; i < num_packets; i++) {
            delete packet_blocks[i];
        }
    }

    if(info_blocks) {
        for(size_t i = 0; i < num_packets; i++) {
            delete info_blocks[i];
        }
    }

    if(master_block) {
        delete master_block;
    }

    if(last_nonces) {
        free(last_nonces);
    }
}

RetType TelemetryShm::init(VCM* vcm) {
    num_packets = vcm->num_packets;

    // create and zero last_nonces
    last_nonces = (uint32_t*)malloc(num_packets * sizeof(uint32_t));
    memset(last_nonces, 0, num_packets * sizeof(uint32_t));

    // create master Shm object
    // use an id guaranteed unused so we can use the same file name for all blocks
    master_block = new Shm(vcm->config_file.c_str(), 0, sizeof(shm_info_t));

    // create Shm objects for each telemetry packet
    packet_blocks = new Shm*[num_packets];
    info_blocks = new Shm*[num_packets];

    packet_info_t* packet;
    size_t i;
    for(i = 0; i < num_packets; i++) {
        packet = vcm->packets[i];
        // for shmem id use (i+1)*2 for packets (always even) and (2*i)+1 for info blocks (always odd)
        // guarantees all blocks can use the same file but different ids to make a key
        packet_blocks[i] = new Shm(vcm->config_file.c_str(), 2*(i+1), packet->size);
        info_blocks[i] = new Shm(vcm->config_file.c_str(), (2*i)+1, sizeof(uint32_t)); // holds one nonce
    }

    return SUCCESS;
}

RetType TelemetryShm::init() {
    VCM vcm; // leave this on stack to be discarded after return
    return init(&vcm);
}

RetType TelemetryShm::open() {
    for(size_t i = 0; i < num_packets; i++) {
        if(SUCCESS != packet_blocks[i]->attach()) {
            return FAILURE;
        }
        else if(SUCCESS != info_blocks[i]->attach()) {
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
        }

        // attach first
        if(info_blocks[i]->attach() == FAILURE) {
            logger.log_message("failed to attach to shared memory block");
            return FAILURE;
        }

        // 'info blocks' are single nonces now, so just zero them
        *((uint32_t*)info_blocks[i]->data) = 0x0;

        // we should unatach after setting the default
        // although we technically still could stay attached and be okay
        if(info_blocks[i]->detach() == FAILURE) {
            logger.log_message("failed to detach from shared memory block");
            return FAILURE;
        }

        // info blocks are just nonces now, only one lock stored in the master block
        // shm_info_t* info = (shm_info_t*)info_blocks[i]->data;
        //
        // // initialize info block
        // // init semaphores
        // INIT(info->rmutex, 1);
        // INIT(info->wmutex, 1);
        // INIT(info->readTry, 1);
        // INIT(info->resource, 1);
        //
        // // init reader/writer counts to 0
        // info->readers = 0;
        // info->writers = 0;
        //
        // // start the nonce at 0
        // info->nonce = 0;
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
    Shm* packet = packet_blocks[packet_id];
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
    Shm* packet = packet_blocks[packet_id];
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

RetType TelemetryShm::read_lock(unsigned int* packet_ids, size_t num, uint32_t timeout) {
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

    // set timeout if there is one
    // NOTE: we use an absolute value for 'timespec' NOT relative
    // see 'man futex' under FUTEX_WAIT section
    struct timespec* timespec = NULL;
    if(timeout > 0) {
        struct timeval curr_time;
        gettimeofday(&curr_time, NULL);

        uint32_t ms = curr_time.tv_sec * 1000;
        ms += curr_time.tv_usec / 1000;
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

        // the only time this is zero is if we received a signal at some point
        // if we continue on, we will block instead of exiting
        if(last_nonce == 0) {
            // we should just unlock and exit
            logger.log_message("received signal, exiting early");
            read_unlock(true);
            return INTERRUPTED;
        }

        // if reading in standard mode we never block so don't check
        // actually we want to update our last nonces so keep going for now
        // if(read_mode == STANDARD_READ) {
        //     return SUCCESS;
        // }

        // check to see if any nonce has changed for the packets we're locking
        // if any nonce has changed we don't need to block
        uint32_t bitset = 0;
        uint32_t* nonce;
        unsigned int id;
        bool block = true; // whether or not we block
        for(size_t i = 0; i < num; i++) {
            bitset |= (1 << i);

            id = packet_ids[i];

            nonce = (uint32_t*)(info_blocks[id]->data);
            if(*nonce != last_nonces[id]) {
                // we found a nonce that changed!
                // important to not just return here since we may have other stored nonces to update
                last_nonces[id] = *nonce;
                block = false;;
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
                }

                // if we get EAGAIN, it could mean that the nonce changed before we could block, go back and check again
                if(errno != EAGAIN) {
                    // else something bad
                    return FAILURE;
                }
            } // otherwise we've been woken up

            if(info->nonce == 0) {
                // if this is 0, we got a signal and should exit
                logger.log_message("woke up after signal, exiting");
                return INTERRUPTED;
            }
            // if(force_woken) {
            //     // we woke up because someone forced us, not because memory updated
            //     force_woken = false; // reset in case the user decides to try and block again
            //     return FAILURE;
            // }
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

    // set timeout if there is one
    // NOTE: we use an absolute value for 'timespec' NOT relative
    // see 'man futex' under FUTEX_WAIT section
    struct timespec* timespec = NULL;
    if(timeout > 0) {
        struct timeval curr_time;
        gettimeofday(&curr_time, NULL);

        uint32_t ms = curr_time.tv_sec * 1000;
        ms += curr_time.tv_usec / 1000;
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
            // update the master nonce and leave
            last_nonce = info->nonce;
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
                if(-1 == syscall(SYS_futex, &info->nonce, FUTEX_WAIT_BITSET, last_nonce, timespec, NULL, 0xFF)) {
                    if(errno == ETIMEDOUT) {
                        logger.log_message("shared memory wait timed out");
                    }

                    // if we get EAGAIN, it could mean that the nonce changed before we could block, go back and check again
                    if(errno != EAGAIN) {
                        // else something bad
                        return FAILURE;
                    }
                } // otherwise we've been woken up

                if(info->nonce == 0) {
                    // if this is 0, we got a signal and should exit
                    logger.log_message("woke up after signal, exiting");
                    return INTERRUPTED;
                }
                // if(force_woken) {
                //     // we woke up because someone forced us, not because memory updated
                //     force_woken = false; // reset in case the user decides to try and block again
                //     return FAILURE;
                // }
            } else {
                return BLOCKED;
            }
        } else {
            // update all the stored packet nonces
            for(size_t i = 0; i < num_packets; i++) {
                last_nonces[i] = *((uint32_t*)(info_blocks[i]->data));
            }

            // update the master nonce
            last_nonce = info->nonce;

            read_locked = true;
            return SUCCESS;
        }

    }
}

RetType TelemetryShm::force_wake(uint32_t mask) {
    MsgLogger logger("TelemetryShm", "force_wake");

    if(packet_blocks == NULL || info_blocks == NULL || master_block == NULL) {
        // not open
        logger.log_message("object not open");
        return FAILURE;
    }

    // packet_id is an index
    shm_info_t* info = (shm_info_t*)master_block->data;

    if(info == NULL) {
        logger.log_message("shared memory block is null");
        // something isn't attached
        return FAILURE;
    }

    // wakeup anyone blocked on this packet (or any packet with an equivalent id mod 32)
    // this includes ourself if we are sleeping!
    // everyone else should get woken and see that the nonce hasn't changed, so they will go back to sleep
    syscall(SYS_futex, &(info->nonce), FUTEX_WAKE_BITSET, INT_MAX, NULL, NULL, mask); // TODO check return

    return SUCCESS;
}

// handle a signal, should be called from a sighandler
void TelemetryShm::sighandler() {
    MsgLogger logger("TelemetryShm", "sighandler");

    if(master_block == NULL) {
        // we aren't attached, just return
        return;
    }

    shm_info_t* info = (shm_info_t*)master_block->data;
    void* futex_word = &(info->nonce);

    if(FAILURE == master_block->detach()) {
        logger.log_message("Failed to detach from shm");
        // we shouldn't do anything else, trying to stop the process failed and we'll stay running
        return;
    }

    // remap the futex word virtual address and then change it
    // it will look like the nonce changed when the syscall restarts but it didn't
    // change in shared memory
    void* addr = mmap(futex_word, 4, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);

    if(addr != futex_word) {
        // mmap failed to map to the address we wanted, leave process running
        logger.log_message("mmap failed to map to correct address");
        return;
    }

    // set nonce to zero so futex_wait returns EAGAIN on the restart
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

const uint8_t* TelemetryShm::get_buffer(uint32_t packet_id) {
    MsgLogger logger("TelemtryShm", "get_buffer");

    if(packet_id >= num_packets) {
        logger.log_message("invalid packet id");
        return NULL;
    }

    if(packet_blocks == NULL) {
        logger.log_message("shared memory block is null");
        return NULL;
    }

    return packet_blocks[packet_id]->data;
}

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

    uint32_t* nonce = (uint32_t*)(info_blocks[packet_id]->data);
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
