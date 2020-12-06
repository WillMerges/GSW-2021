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
#include "lib/shm/shm.h"
#include "common/types.h"

namespace shm {

    void* shmem = NULL;
    size_t size = 1024; // default
    int shmid = -1;

    void set_size(size_t s) {
        size = s;
    }

    RetType attach_to_shm() {
        key_t key = ftok(file, id);
        shmid = shmget(key, size, 0666|IPC_CREAT);
        shmem = shmat(shmid, (void*)0, 0);

        return SUCCESS;
    }

    RetType detach_from_shm() {
        if(shmem) {
            shmdt(shmem);
            return SUCCESS;
        }
        return FAILURE;
    }

    RetType destroy_shm() {
        if(shmid == -1) {
            return FAILURE;
        }
        shmctl(shmid, IPC_RMID, NULL);
        return SUCCESS;
    }

    void* get_shm_block() {
        return shmem;
    }

}
