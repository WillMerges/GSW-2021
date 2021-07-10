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

RetType TelemetryShm::init(VCM vcm) {

    // creates and zero last_nonces, TODO after num_packets is set
    last_nonces = (uint32_t*)malloc(num_packets * sizeof(uint32_t));
    memset(last_nonces, 0, num_packets * sizeof(uint32_t));

    // TODO setup shm block objects in list
    return FAILURE;
}

RetType TelemetryShm::init() {
    VCM vcm; // leave this on stack to be discarded after return
    return init(vcm);
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

        shm_info_t* info = (shm_info_t*)info_blocks[i]->data;

        // initialize info block
        // init semaphores
        INIT(info->rmutex, 1);
        INIT(info->wmutex, 1);
        INIT(info->readTry, 1);
        INIT(info->resource, 1);

        // init reader/writer counts to 0
        info->readers = 0;
        info->writers = 0;

        // start the nonce at 0
        info->nonce = 0;
    }

    if(SUCCESS != master_block->create()) {
        return FAILURE;
    }

    // start the master nonce at 0
    *((uint32_t*) master_block->data) = 0;

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
    if(packet_id > num_packets) {
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
    shm_info_t* info = (shm_info_t*)info_blocks[packet_id]->data;

    if(packet == NULL || info == NULL) {
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
    info->nonce++; // update the nonce

    // since readers only read this nonce atomically (using futex), it's okay
    // if two writers update this at the same time since the nonce will still change
    (*((uint32_t*)master_block->data))++; // update the master nonce

    // wakeup anyone blocked on this packet
    syscall(SYS_futex, &(info->nonce), FUTEX_WAKE_BITSET, INT_MAX, NULL, NULL, 1 << packet_id); // TODO check return

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
    if(packet_id > num_packets) {
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
    shm_info_t* info = (shm_info_t*)info_blocks[packet_id]->data;

    if(packet == NULL || info == NULL) {
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
    info->nonce++; // update the nonce

    // since readers only read this nonce atomically (using futex), it's okay
    // if two writers update this at the same time since the nonce will still change
    (*((uint32_t*)master_block->data))++; // update the master nonce

    // wakeup anyone blocked on this packet
    // technically we can only block on up to 32 packets, but we mod the packet id so that
    // some packets may have to share, the reader should check to see if their packet really updated
    syscall(SYS_futex, master_block->data, FUTEX_WAKE_BITSET, INT_MAX, NULL, NULL, 1 << (packet_id % 32));

    V(info->resource);

    P(info->wmutex);
    info->writers--;
    if(info->writers == 0) {
        V(info->readTry);
    }
    V(info->wmutex);

    return SUCCESS;
}

// TODO update to work with multiple packet ids
RetType TelemetryShm::read_lock(unsigned int packet_id) {
    if(packet_id > num_packets) {
        // TODO sys message
        return FAILURE;
    }

    if(info_blocks == NULL || master_block == NULL) {
        // not open
        // TODO sys message
        return FAILURE;
    }

    // packet_id is an index
    shm_info_t* info = (shm_info_t*)info_blocks[packet_id]->data;

    if(info == NULL) {
        // TODO sys message
        // something isn't attached
        return FAILURE;
    }

    uint32_t last_nonce = last_nonces[packet_id];

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

        if(STANDARD_READ == read_mode) {
            return SUCCESS;
        }

        if(last_nonce == info->nonce) { // no update
            if(BLOCKING_READ == read_mode) {
                return BLOCKED;
            }
            else { // block
                read_unlock(packet_id);
                syscall(SYS_futex, master_block->data, FUTEX_WAIT, last_nonce, NULL, NULL, 1 << (packet_id % 32));
            }
        } else { // updated since last read
            return SUCCESS;
        }
    }
}

RetType TelemetryShm::read_unlock(unsigned int packet_id) {
    if(packet_id > num_packets) {
        // TODO sys message
        return FAILURE;
    }

    if(info_blocks == NULL || master_block == NULL) {
        // not open
        // TODO sys message
        return FAILURE;
    }

    // packet_id is an index
    shm_info_t* info = (shm_info_t*)info_blocks[packet_id]->data;

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

// TODO for read, make a "read_lock" and "read_unlock" function instead of copying in/out
// a read_lock can block on multiple packets, lock the master uint32 using FUTEX_WAIT_BITSET
// where each bit corresponds to a packet (32 packets max)
// the writers use FUTEX_WAKE_BITSET with the bit for the packet set updated

#undef P
#undef V
#undef INIT
