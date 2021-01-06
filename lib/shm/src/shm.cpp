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
#include "lib/shm/shm.h"
// #include "lib/dls/dls.h" // TODO add logging (make sure to change startup order)
#include "common/types.h"

// TODO add some better error checking


// TODO
// use shared mem and put pthread mutexes/semaphores in it to handle locking
// lock readers out on write, lock writers out on read
// sample code:
//
// read_shmem()
//   lock on write mutex
//   inc read semaphore
//   copy shmem into buffer
//   dec read semaphore
//   unlock write mutex

// write_shmem()
//   lock on write mutex
//   block until read semaphore = 0 (doesn't allow new readers)
//   copy buffer into shared mem
//   unlock write mutex

// TODO
// change shared mem id
// if running multiple GSW instances they will try to create the same shared mem
// maybe hash some file location (like GSW_HOME?)
// or hash VCM file location (so could run multiple configs in same GSW at same time?)

namespace shm {

    void* shmem = NULL;
    size_t shm_size = 1024; // default
    int shmid = -1;

    // TODO add locking
    RetType write_to_shm(void* src, size_t size) {
        if(!shmem) {
            return FAILURE;
        }
        if(size > shm_size) {
            return FAILURE;
        }

        memcpy(shmem, src, size);
        return SUCCESS;
    }

    RetType read_from_shm(void* dst, size_t size) {
        if(!shmem) {
            return FAILURE;
        }
        if(size < shm_size) {
            return FAILURE;
        }

        memcpy(dst, shmem, size);
        return SUCCESS;
    }

    RetType clear_shm() {
        if(shmem) {
            return FAILURE;
        }
        memset(shmem, 0, shm_size);
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
        key_t key = ftok(file, id);
        shmid = shmget(key, shm_size, 0666|IPC_CREAT|IPC_EXCL);
        if(shmid == -1) {
            return FAILURE;
        }

        return SUCCESS;
    }

    RetType attach_to_shm() {
        key_t key = ftok(file, id);
        shmid = shmget(key, shm_size, 0666);
        if(shmid == -1) {
            // perror
            return FAILURE;
        }

        shmem = shmat(shmid, (void*)0, 0);
        if(shmem == (void*) -1) {
            shmem = NULL;
            // perror
            return FAILURE;
        }

        return SUCCESS;
    }

    RetType detach_from_shm() {
        if(shmem) {
            if(shmdt(shmem) == 0) {
                shmem = NULL;
                return SUCCESS;
            }
        }
        return FAILURE;
    }

    RetType destroy_shm() {
        if(shmid == -1) {
            return FAILURE;
        }
        if(shmctl(shmid, IPC_RMID, NULL) == -1) {
            return FAILURE;
        }
        shmem = NULL;
        return SUCCESS;
    }

    // void* get_shm_block() {
    //     return shmem;
    // }

}
