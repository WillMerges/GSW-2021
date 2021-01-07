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
#include "lib/shm/shm.h"
#include "lib/vcm/vcm.h"
// #include "lib/dls/dls.h" // TODO add logging (make sure to change startup order)
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


namespace shm {

    // VCM (for size and file name)
    vcm::VCM* vcm = NULL;

    // nonce to check for updates
    unsigned int last_nonce = 0; // this will wrap around, but that's fine

    // info block for locking shared memory
    typedef struct {
        unsigned int nonce;
        unsigned int readers;
        unsigned int writers;
        sem_t rmutex;
        sem_t wmutex;
        sem_t readTry;
        sem_t resource;
        pthread_mutex_t nonce_lock;
        pthread_cond_t nonce_cond;
    } shm_info_t;

    // info block
    shm_info_t* info = NULL;
    int info_shmid = -1;

    // main shmem block
    void* shmem = NULL;
    int shmid = -1;


    // reading and writing is done with *writers-preference*
    // https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem
    RetType write_to_shm(void* src, size_t size, size_t offset) {
        if(!shmem || !info) {
            return FAILURE;
        }
        if((size + offset) > vcm->packet_size) {
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
        pthread_cond_broadcast(&(info->nonce_cond)); // TODO error check return value

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
    RetType read_from_shm(void* dst, size_t size, size_t offset) {
        if(!shmem || !info) {
            return FAILURE;
        }
        if((size + offset) < vcm->packet_size) {
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

    RetType read_from_shm_if_updated(void* dst, size_t size, size_t offset) {
        RetType ret = SUCCESS;

        if(!shmem || !info) {
            return FAILURE;
        }
        if((size + offset) < vcm->packet_size) {
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
    RetType read_from_shm_block(void* dst, size_t size, size_t offset) {
        if(!shmem || !info) {
            return FAILURE;
        }
        if((size + offset) < vcm->packet_size) {
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

                // we can guarantee no one changed the nonce so we can use a 'fake' lock
                // only the writer can change the nonce
                // so block here
                pthread_cond_wait(&(info->nonce_cond), &(info->nonce_lock)); // TODO error check return value
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

    // locking same as write
    RetType clear_shm() {
        if(!shmem || !info) {
            return FAILURE;
        }

        P(info->wmutex);
        info->writers++;
        if(info->writers == 1) {
            P(info->readTry);
        }
        V(info->wmutex);
        P(info->resource);

        memset(shmem, 0, vcm->packet_size);
        info->nonce++; // update the nonce

        V(info->resource);

        P(info->wmutex);
        info->writers--;
        if(info->writers == 0) {
            V(info->readTry);
        }
        V(info->wmutex);

        return SUCCESS;
    }

    size_t get_shmem_size() {
        if(vcm) {
            return vcm->packet_size;
        }
        return 0;
    }

    RetType create_shm(vcm::VCM* vcm) {
        // create info shmem
        key_t info_key = ftok(vcm->config_file.c_str(), info_id);
        if(info_key == (key_t) -1) {
            return FAILURE;
        }

        info_shmid = shmget(info_key, sizeof(shm_info_t),
                            0666|IPC_CREAT|IPC_EXCL);
        if(info_shmid == -1) {
            return FAILURE;
        }

        // briefly attach to info shmem
        info = (shm_info_t*) shmat(info_shmid, (void*)0, 0);
        if(info == (void*) -1) {
            info = NULL;
            return FAILURE;
        }

        // init semaphores
        INIT(info->rmutex, 1);
        INIT(info->wmutex, 1);
        INIT(info->readTry, 1);
        INIT(info->resource, 1);

        // init a dummy lock to use with the nonce
        info->nonce_lock = PTHREAD_MUTEX_INITIALIZER;

        // init a shared condition variable to block for updates
        // TODO check the returns of these for errors
        pthread_condattr_t cattr;
        pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&(info->nonce_cond), &cattr);

        // init reader/writer counts to 0
        info->readers = 0;
        info->writers = 0;

        // start the nonce at 0
        info->nonce = 0;

        // detach from info shmem
        if(shmdt(info) != 0) {
            return FAILURE;
        }
        info = NULL;

        // set up shmem
        key_t key = ftok(vcm->config_file.c_str(), id);
        if(key == (key_t) -1) {
            return FAILURE;
        }

        // shmid = shmget(key, vcm->packet_size, 0666|IPC_CREAT|IPC_EXCL);
        shmid = shmget(key, vcm->packet_size, 0666|IPC_CREAT|IPC_EXCL);
        if(shmid == -1) {
            return FAILURE;
        }

        return SUCCESS;
    }

    RetType attach_to_shm(vcm::VCM* selected_vcm) {
        vcm = selected_vcm; // hopefully this doesn't get deleted

        key_t info_key = ftok(vcm->config_file.c_str(), info_id);
        if(info_key == (key_t) -1) {
            return FAILURE;
        }

        info_shmid = shmget(info_key, sizeof(shm_info_t), 0666);
        if(info_shmid == -1) {
            return FAILURE;
        }

        info = (shm_info_t*) shmat(info_shmid, (void*)0, 0);
        if(info == (void*) -1) {
            info = NULL;
            return FAILURE;
        }

        key_t key = ftok(vcm->config_file.c_str(), id);
        if(key == (key_t) -1) {
            return FAILURE;
        }

        shmid = shmget(key, vcm->packet_size, 0666);
        if(shmid == -1) {
            return FAILURE;
        }

        shmem = shmat(shmid, (void*)0, 0);
        if(shmem == (void*) -1) {
            shmem = NULL;
            return FAILURE;
        }

        return SUCCESS;
    }

    RetType detach_from_shm() {
        if(info) {
            if(shmdt(info) != 0) {
                return FAILURE;
            }
            info = NULL;
        }

        if(shmem) {
            if(shmdt(shmem) == 0) {
                shmem = NULL;
                vcm = NULL;
                return SUCCESS;
            }
        }
        return FAILURE;
    }

    RetType destroy_shm() {
        RetType ret = SUCCESS;

        if(shmid == -1 || info_shmid == -1) {
            ret = FAILURE;
        }

        if(shmctl(info_shmid, IPC_RMID, NULL) == -1) {
            ret = FAILURE;
        } else {
            info_shmid = -1;
            info = NULL;
        }

        if(shmctl(shmid, IPC_RMID, NULL) == -1) {
            ret = FAILURE;
        } else {
            shmid = -1;
            shmem = NULL;
        }

        // only reset vcm if
        if(ret == SUCCESS) {
            vcm = NULL;
        }

        return ret;
    }
}

#undef P
#undef V
#undef INIT
