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
    // need to create a list of Shm object pointers stored in packet_blocks
    // should be num_packets many blocks of the correct size according to the vcm file
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
        // else if(SUCCESS != info_blocks[i]->attach()) {
        //     return FAILURE;
        // }
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
        // else if(SUCCESS != info_blocks[i]->detach()) {
        //     return FAILURE;
        // }
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
        // else if(SUCCESS != info_blocks[i]->create()) {
        //     return FAILURE;
        // }

        // shm_info_t* info = (shm_info_t*)info_blocks[i]->data;

        // initialize info block
        // init semaphores
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

    // start the nonce at 0
    info->nonce = 0;


    // start the master nonce at 0
    // *((uint32_t*) master_block->data) = 0;

    return SUCCESS;
}

// NOTE: must be attached already!
RetType TelemetryShm::destroy() {
    for(size_t i = 0; i < num_packets; i++) {
        if(SUCCESS != packet_blocks[i]->destroy()) {
            return FAILURE;
        }
        // else if(SUCCESS != info_blocks[i]->destroy()) {
        //     return FAILURE;
        // }
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

    // if(packet_blocks == NULL || info_blocks == NULL || master_block == NULL) {
    if(packet_blocks == NULL || master_block == NULL) {
        // not open
        // TODO sys message
        return FAILURE;
    }

    // packet_id is an index
    Shm* packet = packet_blocks[packet_id];
    // shm_info_t* info = (shm_info_t*)info_blocks[packet_id]->data;
    shm_info_t* info = (shm_info_t*)master_block->data;

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
    // (*((uint32_t*)master_block->data))++; // update the master nonce

    // wakeup anyone blocked on this packet (actually any packet now!)
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
    if(packet_id > num_packets) {
        // TODO sys message
        return FAILURE;
    }

    // if(packet_blocks == NULL || info_blocks == NULL || master_block == NULL) {
    if(packet_blocks == NULL || master_block == NULL) {
        // not open
        // TODO sys message
        return FAILURE;
    }

    // packet_id is an index
    Shm* packet = packet_blocks[packet_id];
    // shm_info_t* info = (shm_info_t*)info_blocks[packet_id]->data;
    shm_info_t* info = (shm_info_t*)master_block->data;

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
    // (*((uint32_t*)master_block->data))++; // update the master nonce

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

// no longer used since all packets share the same lock
// never really worked fully
// RetType TelemetryShm::read_lock(unsigned int* packet_ids, size_t num_packets) {
//     RetType ret;
//     RetType stat;
//
//     while(1) {
//         ret = FAILURE;
//
//         for(size_t i = 0; i < num_packets; i++) {
//             stat = read_lock(packet_ids[i]);
//
//             switch(stat) {
//                 case FAILURE:
//                     // unlock everything we locked
//                     read_unlock(packet_ids, i);
//                     return FAILURE;
//                 case SUCCESS:
//                     ret = SUCCESS;
//                     break;
//                 case BLOCKED:
//                     if(read_mode != STANDARD_READ) {
//                         if(ret != SUCCESS) { // no successful reads
//                             ret = BLOCKED;
//                         }
//                     }
//                     break;
//             }
//         }
//
//         if(ret == BLOCKED && read_mode == BLOCKING_READ) {
//             read_unlock(packet_ids, num_packets);
//
//             uint32_t mask = 0;
//             for(size_t i = 0; i < num_packets; i++) {
//                 mask |= (1 << (packet_ids[i] % 32));
//             }
//
//             // sleep until anything updates
//             syscall(SYS_futex, master_block->data, FUTEX_WAIT_BITSET, *master_block->data, NULL, NULL, mask);
//         } else {
//             return ret;
//         }
//     }
// }

// used to pass in a packet_id to this function
// lock is shared for all packets now so no longer do this
RetType TelemetryShm::read_lock() {
    // if(packet_id > num_packets) {
    //     // TODO sys message
    //     return FAILURE;
    // }

    // if(info_blocks == NULL || master_block == NULL) {
    if(master_block == NULL) {
        // not open
        // TODO sys message
        return FAILURE;
    }

    // packet_id is an index
    // shm_info_t* info = (shm_info_t*)info_blocks[packet_id]->data;
    shm_info_t* info = (shm_info_t*)master_block->data;

    if(info == NULL) {
        // TODO sys message
        // something isn't attached
        return FAILURE;
    }

    // uint32_t last_nonce = last_nonces[packet_id];

    // enter as a reader
    P(info->readTry);
    P(info->rmutex);
    info->readers++;
    if(info->readers == 1) {
        P(info->resource);
    }
    V(info->rmutex);
    V(info->readTry);

    // get the master nonce now
    // this nonce can technically change before we call futex since there are other writers on different telemetry blocks
    // if it does change before we call futex, futex will immediately return and we have to start the check again
    // absolute worst case, we never block the process since writers keep interrupting us, until the writer we're
    // actually waiting on updates and we return since the that block's nonce changed
    // so technically this is probably a bad idea if there are lots of telemetry blocks
    // this is actually no better or worse than just all readers/writers sharing a lock, which is maybe not a bad strategy
    // better? strategy is to not use a master nonce and block on multiple individual nonces using threads
    // ^^^ have to weih if the threading overhead outweighs the performance of having multiple locks
    // maybe make this a selectable option? e.g. like set block mode parallel or single
    // could have the class make this decision depening on how many telemetry packets in the vcm file
    // I suspect for lots of writers (many telemetry packets) the overhead of threads is worth it
    // since you have less readers performing checks only to have the blocks theyre waiting for not be updated
    // but for very few writers doing the checks is probably cheaper than creating threads

    // TODO for now we're only going to lock on a single master nonce, but use bitset waking
    // potentially in the future we can use threads and lock on several packet nonces
    // uint32_t master_nonce = *((uint32_t*)master_block->data);

    if(last_nonce == info->nonce) { // no update
        return BLOCKED;
        if(BLOCKING_READ == read_mode) {
            return BLOCKED;
        }
        else { // block
            read_unlock(packet_id);

            // wait on this packet anyone waiting on this packet
            // (or any packet with an index of a multiple of 32)
            syscall(SYS_futex, info->nonce, FUTEX_WAIT_BITSET, last_nonce, NULL, NULL, 1 << (packet_id % 32));
        }
    } else { // updated since last read
        return SUCCESS;
    }
}

// unlock several packets
// unused now because all packets share the single master block for locking
// RetType TelemetryShm::read_unlock(unsigned int* packet_ids, size_t num_packets) {
//     RetType stat = SUCCESS;
//
//     for(size_t i = 0; i < num_packets; i++) {
//         if(FAILURE == read_unlock(packet_ids[i])) {
//             stat = FAILURE;
//             // TODO logging message
//         }
//     }
//
//     return stat;
// }

// used to pass in a packet_id to this function
// lock is shared for all packets now so no longer do this
RetType TelemetryShm::read_unlock() {
    // if(packet_id > num_packets) {
    //     // TODO sys message
    //     return FAILURE;
    // }

    // if(info_blocks == NULL || master_block == NULL) {
    if(master_block == NULL) {
        // not open
        // TODO sys message
        return FAILURE;
    }

    // packet_id is an index
    // shm_info_t* info = (shm_info_t*)info_blocks[packet_id]->data;
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

// TODO for read, make a "read_lock" and "read_unlock" function instead of copying in/out
// a read_lock can block on multiple packets, lock the master uint32 using FUTEX_WAIT_BITSET
// where each bit corresponds to a packet (32 packets max)
// the writers use FUTEX_WAKE_BITSET with the bit for the packet set updated

// TODO still want to lock on specific packets for more specific return...
// unlock doesnt matter

#undef P
#undef V
#undef INIT
