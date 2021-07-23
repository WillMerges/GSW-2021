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
#include "lib/telemetry/TelemetryShm.h"

// TODO make packet ids be uint32_ts to match VCM

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
    last_nonce = 0;
    read_mode = STANDARD_READ;
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

    // creates and zero last_nonces
    last_nonces = (uint32_t*)malloc(num_packets * sizeof(uint32_t));
    memset(last_nonces, 0, num_packets * sizeof(uint32_t));

    packet_blocks = new Shm*[num_packets];
    info_blocks = new Shm*[num_packets];

    // create Shm objects for each telemetry packet
    packet_info_t* packet;
    size_t i;
    for(i = 0; i < num_packets; i++) {
        packet = vcm->packets[i];
        // for shmem id use (i+1)*2 for packets (always even) and (2*i)+1 for info blocks (always odd)
        // guarantees all blocks can use the same file but different ids to make a key
        packet_blocks[i] = new Shm(vcm->config_file.c_str(), 2*(i+1), packet->size);
        info_blocks[i] = new Shm(vcm->config_file.c_str(), (2*i)+1, sizeof(uint32_t)); // holds one nonce
    }

    // create master Shm object
    // use an id guaranteed unused so we can use the same file name for master block as well
    master_block = new Shm(vcm->config_file.c_str(), 2*(i+1), sizeof(shm_info_t));

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
    for(size_t i = 0; i < num_packets; i++) {
        if(SUCCESS != packet_blocks[i]->create()) {
            return FAILURE;
        }
        else if(SUCCESS != info_blocks[i]->create()) {
            return FAILURE;
        }

        // attach first
        if(info_blocks[i]->attach() == FAILURE) {
            // TODO sysm
            return FAILURE;
        }

        // 'info blocks' are single nonces now, so just zero them
        *((uint32_t*)info_blocks[i]->data) = 0x0;

        // we should unatach after setting the default
        // although we technically still could stay attached and be okay
        if(info_blocks[i]->detach() == FAILURE) {
            // TODO sysm
            return FAILURE;
        }

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
        // TODO sysm
        return FAILURE;
    }

    // need to attach in order to preset data
    if(SUCCESS != master_block->attach()) {
        // TODO sysm
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

    // start the master nonce at 0
    info->nonce = 0;

    // we should detach to be later attached
    // if this fails it's not the end of the world? but its still bad and shouldn't fail
    if(SUCCESS != master_block->detach()) {
        // TODO sysm
        return FAILURE;
    }

    return SUCCESS;
}

// NOTE: must be attached already!
RetType TelemetryShm::destroy() {
    for(size_t i = 0; i < num_packets; i++) {
        if(SUCCESS != packet_blocks[i]->destroy()) {
            return FAILURE;
        }
        else if(SUCCESS != info_blocks[i]->destroy()) {
            return FAILURE;
        }
    }

    if(SUCCESS != master_block->destroy()) {
        return FAILURE;
    }

    return SUCCESS;
}

RetType TelemetryShm::write(unsigned int packet_id, uint8_t* data) {
    if(packet_id >= num_packets) {
        // TODO sys message
        return FAILURE;
    }

    if(packet_blocks == NULL || info_blocks == NULL || master_block == NULL) {
        // not open
        // TODO sys message
        return FAILURE;
    }

    // packet_id is an index
    Shm* packet = packet_blocks[packet_id];
    uint32_t* packet_nonce = (uint32_t*)info_blocks[packet_id]->data;
    shm_info_t* info = (shm_info_t*)master_block->data;

    if(packet == NULL || info == NULL || packet_nonce == NULL) {
        // TODO sys message
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

    (*packet_nonce)++; // update the packet nonce

    // wakeup anyone blocked on this packet (or any packet with an equivalen id mod 32)
    syscall(SYS_futex, &(info->nonce), FUTEX_WAKE_BITSET, INT_MAX, NULL, NULL, 1 << (packet_id % 32)); // TODO check return

    V(info->resource);

    P(info->wmutex);
    info->writers--;
    if(info->writers == 0) {
        V(info->readTry);
    }
    V(info->wmutex);

    return SUCCESS;
}

RetType TelemetryShm::clear(unsigned int packet_id, uint8_t val) {
    if(packet_id >= num_packets) {
        // TODO sys message
        return FAILURE;
    }

    if(packet_blocks == NULL || info_blocks == NULL || master_block == NULL) {
        // not open
        // TODO sys message
        return FAILURE;
    }

    // packet_id is an index
    Shm* packet = packet_blocks[packet_id];
    uint32_t* packet_nonce = (uint32_t*)info_blocks[packet_id]->data;
    shm_info_t* info = (shm_info_t*)master_block->data;

    if(packet == NULL || info == NULL || packet_nonce == NULL) {
        // TODO sys message
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

    (*packet_nonce)++; // update the packet nonce

    // wakeup anyone blocked on this packet
    // technically we can only block on up to 32 packets, but we mod the packet id so that
    // some packets may have to share, the reader should check to see if their packet really updated
    syscall(SYS_futex, info->nonce, FUTEX_WAKE_BITSET, INT_MAX, NULL, NULL, 1 << (packet_id % 32));

    V(info->resource);

    P(info->wmutex);
    info->writers--;
    if(info->writers == 0) {
        V(info->readTry);
    }
    V(info->wmutex);

    return SUCCESS;
}

RetType TelemetryShm::read_lock(unsigned int* packet_ids, size_t num) {
    if(read_locked) {
        // already locked
        return FAILURE;
    }

    if(info_blocks == NULL || master_block == NULL) {
        // not open
        // TODO sys message
        return FAILURE;
    }

    shm_info_t* info = (shm_info_t*)master_block->data;

    if(info == NULL) {
        // TODO sys message
        // something isn't attached
        return FAILURE;
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

        // if reading in standard mode we never block so don't check
        if(read_mode == STANDARD_READ) {
            return SUCCESS;
        }

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

        if(!block) {
            return SUCCESS;
        }

        // if we made it here none of our nonces changed :(
        // time to block
        read_unlock();

        if(read_mode == BLOCKING_READ) {
            syscall(SYS_futex, info->nonce, FUTEX_WAIT_BITSET, last_nonce, NULL, NULL, bitset);
        } else { // NONBLOCKING_READ
            return BLOCKED;
        }
    }
}

// TODO packet nonces aren't being updated here which may cause problems
// see about updating all nonces in read_unlock?? (maybe except master nonce since it's needed for locking)
// ^^^ did the above, check if that's correct...
// I THINK IT'S WRONG then next time you call lock and it was technically updated it blocks...
// so they should probably all be updated here and only the ones being checked in the other version
RetType TelemetryShm::read_lock() {
    if(info_blocks == NULL || master_block == NULL) {
        // not open
        // TODO sys message
        return FAILURE;
    }

    shm_info_t* info = (shm_info_t*)master_block->data;

    if(info == NULL) {
        // TODO sys message
        // something isn't attached
        return FAILURE;
    }

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
        return SUCCESS;
    }

    if(last_nonce == info->nonce) { // nothing changed, block
        if(read_mode == BLOCKING_READ) {
            // wait for any packet to be updated
            // we don't need to loop here and check if it was our packet that updated since we don't care which packet updated
            syscall(SYS_futex, info->nonce, FUTEX_WAIT_BITSET, last_nonce, NULL, NULL, 0xFF);
        } else {
            return BLOCKED;
        }
    }

    // update all the stored packet nonces
    for(size_t i = 0; i < num_packets; i++) {
        last_nonces[i] = *((uint32_t*)(info_blocks[i]->data));
    }

    return SUCCESS;
}

RetType TelemetryShm::read_unlock() {
    if(info_blocks == NULL || master_block == NULL) {
        // not open
        // TODO sys message
        return FAILURE;
    }

    // packet_id is an index
    shm_info_t* info = (shm_info_t*)master_block->data;

    if(info == NULL) {
        // TODO sys message
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

    return SUCCESS;
}

const uint8_t* TelemetryShm::get_buffer(unsigned int packet_id) {
    if(packet_id >= num_packets) {
        // TODO sysm
        return NULL;
    }

    if(packet_blocks == NULL) {
        // TODO sysm
        return NULL;
    }

    return packet_blocks[packet_id]->data;
}

RetType TelemetryShm::packet_updated(unsigned int packet_id, bool* updated) {
    if(!read_locked) {
        // TODO sysm, not read locked
        return FAILURE;
    }

    if(packet_id >= num_packets) {
        // TODO sysm
        return FAILURE;
    }

    uint32_t* nonce = (uint32_t*)(info_blocks[packet_id]->data);
    *updated = (last_nonces[packet_id] == *nonce);

    return SUCCESS;
}

#undef P
#undef V
#undef INIT
