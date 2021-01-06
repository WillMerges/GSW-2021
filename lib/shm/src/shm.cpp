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

// TODO add some better error checking

// TODO figure out locking for readers and writers
// have a single lock that represents "writing"
// have read/write functions that copy in/out of shmem?
// or just have lock/unlock functions

namespace shm {

    // // process node type
    // typedef struct {
    //     int id;
    //     p_node_t* next;
    // } p_node_t;
    //
    // // shared mem info block
    // typedef struct {
    //     p_node_t* writers;
    //     p_node_t* readers;
    // } shm_info_t;


    void* shmem = NULL;
    size_t size = 1024; // default
    int shmid = -1;

    RetType set_shmem_size(size_t s) {
        if(size > 0) {
            size = s + 1; // add 1 for lock byte
            return SUCCESS;
        }
        return FAILURE;
    }

    size_t get_shmem_size() {
        return size - 1; // subtract 1 for lock byte
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
