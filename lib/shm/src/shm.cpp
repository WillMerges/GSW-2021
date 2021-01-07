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
#include <pthread.h>
// #include <semaphore.h> // don't need (using pthread_mutex_t instead)
#include "lib/shm/shm.h"
// #include "lib/dls/dls.h" // TODO add logging (make sure to change startup order)
#include "common/types.h"

// TODO
// change shared mem id
// if running multiple GSW instances they will try to create the same shared mem
// maybe hash some file location (like GSW_HOME?)
// or hash VCM file location (so could run multiple configs in same GSW at same time?)

// NOTE:
// shared memory can be manually altered with 'ipcs' and 'ipcrm' programs

// P and V semaphore macros
#define P(X) \
    if(0 != pthread_mutex_lock( &( (X) ) )) { \
        return FAILURE; \
    } \

#define V(X) \
    if(0 != pthread_mutex_unlock( &( (X) ) )) { \
        return FAILURE; \
    } \

namespace shm {

    // info block for locking shared memory
    typedef struct {
        unsigned int readers;
        unsigned int writers;
        pthread_mutex_t rmutex;
        pthread_mutex_t wmutex;
        pthread_mutex_t readTry;
        pthread_mutex_t resource;
    } shm_info_t;

    // info block
    shm_info_t* info = NULL;
    int info_shmid = -1;

    // main shmem block
    void* shmem = NULL;
    size_t shm_size = 1024; // default
    int shmid = -1;


    // reading and writing is done with *writers-preference*
    // https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem
    RetType write_to_shm(void* src, size_t size) {
        if(!shmem || !info) {
            return FAILURE;
        }
        if(size > shm_size) {
            return FAILURE;
        }

        P(info->wmutex);
        info->writers++;
        if(info->writers == 1) {
            P(info->readTry);
        }
        V(info->wmutex);
        P(info->resource);

        memcpy(shmem, src, size);

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
    RetType read_from_shm(void* dst, size_t size) {
        if(!shmem || !info) {
            return FAILURE;
        }
        if(size < shm_size) {
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

        memcpy(dst, shmem, size);

        P(info->rmutex);
        info->readers--;
        if(info->readers == 0) {
            V(info->resource);
        }
        V(info->rmutex);

        return SUCCESS;
    }

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

        memset(shmem, 0, shm_size);

        V(info->resource);

        P(info->wmutex);
        info->writers--;
        if(info->writers == 0) {
            V(info->readTry);
        }
        V(info->wmutex);

        return SUCCESS;
    }

    RetType set_shmem_size(size_t s) {
        if(s > 0) {
            shm_size = s;
            return SUCCESS;
        }
        return FAILURE;
    }

    size_t get_shmem_size() {
        return shm_size;
    }

    RetType create_shm() {
        // create info shmem
        key_t info_key = ftok(SHM_FILE, info_id);
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
            // perror
            return FAILURE;
        }

        // set up shmem info
        if(pthread_mutex_init(&(info->rmutex), NULL) != 0) {
            // error creating mutex
            return FAILURE;
        }

        if(pthread_mutex_init(&(info->wmutex), NULL) != 0) {
            // error creating mutex
            return FAILURE;
        }

        if(pthread_mutex_init(&(info->readTry), NULL) != 0) {
            // error creating mutex
            return FAILURE;
        }

        if(pthread_mutex_init(&(info->resource), NULL) != 0) {
            // error creating mutex
            return FAILURE;
        }

        info->readers = 0;
        info->writers = 0;

        // detach from info shmem
        if(shmdt(info) != 0) {
            return FAILURE;
        }
        info = NULL;

        // set up shmem
        key_t key = ftok(SHM_FILE, id);
        if(key == (key_t) -1) {
            return FAILURE;
        }

        shmid = shmget(key, shm_size, 0666|IPC_CREAT|IPC_EXCL);
        if(shmid == -1) {
            return FAILURE;
        }

        return SUCCESS;
    }

    RetType attach_to_shm() {
        key_t info_key = ftok(SHM_FILE, info_id);
        if(info_key == (key_t) -1) {
            return FAILURE;
        }

        info_shmid = shmget(info_key, sizeof(shm_info_t), 0666);
        if(info_shmid == -1) {
            perror("ftok: ");
            return FAILURE;
        }

        info = (shm_info_t*) shmat(info_shmid, (void*)0, 0);
        if(info == (void*) -1) {
            info = NULL;
            return FAILURE;
        }

        key_t key = ftok(SHM_FILE, id);
        if(key == (key_t) -1) {
            return FAILURE;
        }

        shmid = shmget(key, shm_size, 0666);
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

        return ret;
    }
}
