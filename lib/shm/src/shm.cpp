/********************************************************************
*  Name: shm.c
*
*  Purpose: Utility functions for creating/modifying shared memory.
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <pthread.h>
#include <semaphore.h>
#include <linux/futex.h>
#include <unistd.h>
#include <limits.h>
#include <sys/syscall.h>
#include "lib/shm/shm.h"
#include "lib/vcm/vcm.h"
#include "lib/dls/dls.h"
#include "common/types.h"

// NOTE: shared memory can be manually altered with 'ipcs' and 'ipcrm' programs

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


using namespace dls;
using namespace shm;


SHM::SHM(std::string file, size_t size) {
    last_nonce = 0;
    info = NULL;
    info_shmid = -1;
    shmem = NULL;
    shmid = -1;
    shm_size = size;
    file_key = file;
}

// reading and writing is done with *writers-preference*
// https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem
RetType SHM::write_to_shm(void* src, size_t size, uint32_t write_id, size_t offset) {
    MsgLogger logger("SHM", "write_to_shm");

    if(!shmem || !info) {
        logger.log_message("Not attached to shared memory, cannot write");
        return FAILURE;
    }

    if((size + offset) > shm_size) {
        logger.log_message("Size to great to write to shared memory");
        return FAILURE;
    }

    P(info->wmutex);
    info->writers++;
    if(info->writers == 1) {
        P(info->readTry);
    }
    V(info->wmutex);

    P(info->resource);

    memcpy((unsigned char*)shmem + offset, src, size);
    info->nonce++; // update the nonce
    info->write_id = write_id; // update the write id
    syscall(SYS_futex, &(info->nonce), FUTEX_WAKE, INT_MAX, NULL, NULL, 0); // TODO check return

    V(info->resource);

    P(info->wmutex);
    info->writers--;
    if(info->writers == 0) {
        V(info->readTry);
    }
    V(info->wmutex);

    return SUCCESS;
}

// reading and writing is done with *writers-preference*
// https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem
RetType SHM::read_from_shm(void* dst, size_t size, size_t offset) {
    MsgLogger logger("SHM", "read_from_shm");

    if(!shmem || !info) {
        logger.log_message("Not attached to shared memory, cannot read");
        return FAILURE;
    }

    if((size + offset) < shm_size) {
        logger.log_message("Shared memory too large to read into destination buffer");
        return FAILURE;
    }

    P(info->readTry);
    P(info->rmutex);
    info->readers++;
    if(info->readers == 1) {
        P(info->resource);
    }
    V(info->rmutex);
    V(info->readTry);

    memcpy(dst, (unsigned char*)shmem + offset, size);
    last_nonce = info->nonce;

    P(info->rmutex);
    info->readers--;
    if(info->readers == 0) {
        V(info->resource);
    }
    V(info->rmutex);

    return SUCCESS;
}

RetType SHM::read_from_shm_if_updated(void* dst, size_t size, size_t offset) {
    MsgLogger logger("SHM", "read_from_shm_if_updated");

    RetType ret = SUCCESS;

    if(!shmem || !info) {
        logger.log_message("Not attached to shared memory, cannot read");
        return FAILURE;
    }

    if((size + offset) < shm_size) {
        logger.log_message("Shared memory too large to read into destination buffer");
        return FAILURE;
    }

    P(info->readTry);
    P(info->rmutex);
    info->readers++;
    if(info->readers == 1) {
        P(info->resource);
    }
    V(info->rmutex);
    V(info->readTry);

    if(last_nonce == info->nonce) { // no update
        ret = FAILURE;
    } else { // updated, do the read
        memcpy(dst, (unsigned char*)shmem + offset, size);
        last_nonce = info->nonce;
    }

    P(info->rmutex);
    info->readers--;
    if(info->readers == 0) {
        V(info->resource);
    }
    V(info->rmutex);

    return ret;
}

// TODO check if this causes deadlock
RetType SHM::read_from_shm_block(void* dst, size_t size, size_t offset) {
    MsgLogger logger("SHM", "read_from_shm_block");

    if(!shmem || !info) {
        logger.log_message("Not attached to shared memory, cannot read");
        return FAILURE;
    }

    if((size + offset) < shm_size) {
        logger.log_message("Shared memory too large to read into destination buffer");
        return FAILURE;
    }

    int exit = 0;
    //pthread_mutex_lock(&(info->nonce_lock)); // this is fake
    while(!exit) {
        // enter as a reader
        P(info->readTry);
        P(info->rmutex);
        info->readers++;
        if(info->readers == 1) {
            P(info->resource);
        }
        V(info->rmutex);
        V(info->readTry);

        if(last_nonce == info->nonce) { // block
            // leave as a reader
            P(info->rmutex);
            info->readers--;
            if(info->readers == 0) {
                V(info->resource);
            }
            V(info->rmutex);

            // we can guarantee no one changed the nonce so we can block until the nonce changes
            // only the writer can change the nonce
            // so block here
            syscall(SYS_futex, &(info->nonce), FUTEX_WAIT, last_nonce, NULL, NULL, 0); // TODO check return
        } else { // do the read
            memcpy(dst, (unsigned char*)shmem + offset, size);
            last_nonce = info->nonce;
            exit = 1;
        }
    }

    // leave as a reader
    P(info->rmutex);
    info->readers--;
    if(info->readers == 0) {
        V(info->resource);
    }
    V(info->rmutex);

    return SUCCESS;
}

// locking works the same as write
RetType SHM::clear_shm(uint8_t val, uint32_t write_id) {
    MsgLogger logger("SHM", "clear_shm");

    if(!shmem || !info) {
        logger.log_message("Not attached to shared memory, cannot clear");
        return FAILURE;
    }

    P(info->wmutex);
    info->writers++;
    if(info->writers == 1) {
        P(info->readTry);
    }
    V(info->wmutex);
    P(info->resource);

    memset(shmem, val, shm_size);
    info->nonce++; // update the nonce
    info->write_id = write_id; // update the write id
    syscall(SYS_futex, &(info->nonce), FUTEX_WAKE, INT_MAX, NULL, NULL, 0); // TODO check return

    V(info->resource);

    P(info->wmutex);
    info->writers--;
    if(info->writers == 0) {
        V(info->readTry);
    }
    V(info->wmutex);

    return SUCCESS;
}

size_t SHM::get_size() {
    return shm_size;
}

RetType SHM::create_shm() {
    MsgLogger logger("SHM", "create_shm");

    // create info shmem
    key_t info_key = ftok(file_key.c_str(), info_id);
    if(info_key == (key_t) -1) {
        logger.log_message("ftok failure, no key generated for info shmem");
        return FAILURE;
    }

    info_shmid = shmget(info_key, sizeof(shm_info_t),
                        0666|IPC_CREAT|IPC_EXCL);
    if(info_shmid == -1) {
        logger.log_message("shmget failure for info shmem");
        return FAILURE;
    }

    // briefly attach to info shmem
    info = (shm_info_t*) shmat(info_shmid, (void*)0, 0);
    if(info == (void*) -1) {
        info = NULL;
        logger.log_message("shmat failure, cannot attach to info shmem");
        return FAILURE;
    }

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

    // detach from info shmem
    if(shmdt(info) != 0) {
        logger.log_message("shmdt failure, failed to detach from info shmem");
        return FAILURE;
    }
    info = NULL;

    // set up shmem
    key_t key = ftok(file_key.c_str(), id);
    if(key == (key_t) -1) {
        logger.log_message("ftok failure, no key generated");
        return FAILURE;
    }

    // shmid = shmget(key, vcm->packet_size, 0666|IPC_CREAT|IPC_EXCL);
    shmid = shmget(key, shm_size, 0666|IPC_CREAT|IPC_EXCL);
    if(shmid == -1) {
        logger.log_message("shmget failure");
        return FAILURE;
    }

    return SUCCESS;
}

RetType SHM::attach_to_shm() {
    MsgLogger logger("SHM", "attach_to_shm");

    key_t info_key = ftok(file_key.c_str(), info_id);
    if(info_key == (key_t) -1) {
        logger.log_message("ftok failure, no key generated for info shmem");
        return FAILURE;
    }

    info_shmid = shmget(info_key, sizeof(shm_info_t), 0666);
    if(info_shmid == -1) {
        logger.log_message("shmget failure for info shmem");
        return FAILURE;
    }

    info = (shm_info_t*) shmat(info_shmid, (void*)0, 0);
    if(info == (void*) -1) {
        info = NULL;
        logger.log_message("shmat failure, cannot attach to info shmem");
        return FAILURE;
    }

    key_t key = ftok(file_key.c_str(), id);
    if(key == (key_t) -1) {
        logger.log_message("ftok failure, no key generated");
        return FAILURE;
    }

    shmid = shmget(key, shm_size, 0666);
    if(shmid == -1) {
        logger.log_message("shmget failure");
        return FAILURE;
    }

    shmem = shmat(shmid, (void*)0, 0);
    if(shmem == (void*) -1) {
        shmem = NULL;
        logger.log_message("shmat failure");
        return FAILURE;
    }

    return SUCCESS;
}

RetType SHM::detach_from_shm() {
    MsgLogger logger("SHM", "detach_from_shm");

    if(info) {
        if(shmdt(info) != 0) {
            logger.log_message("shmdt failure for info shmem");
            return FAILURE;
        }
        info = NULL;
    }

    if(shmem) {
        if(shmdt(shmem) == 0) {
            shmem = NULL;
            return SUCCESS;
        }
    }
    logger.log_message("shmdt failure");
    return FAILURE;
}

RetType SHM::destroy_shm() {
    MsgLogger logger("SHM", "destroy_shm");

    RetType ret = SUCCESS;

    if(shmid == -1 || info_shmid == -1) {
        logger.log_message("No shmem to destroy");
        ret = FAILURE;
    }

    if(shmctl(info_shmid, IPC_RMID, NULL) == -1) {
        logger.log_message("shmctl failure for info shmem");
        ret = FAILURE;
    } else {
        info_shmid = -1;
        info = NULL;
    }

    if(shmctl(shmid, IPC_RMID, NULL) == -1) {
        logger.log_message("shmctl failure");
        ret = FAILURE;
    } else {
        shmid = -1;
        shmem = NULL;
    }

    return ret;
}

#undef P
#undef V
#undef INIT
