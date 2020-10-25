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

#include "shm.h"

namespace ldms {

    // used for generating shm id
    int id = 65;
    char* file = "shmfile";

    RetType create_shm(size_t size) {
        // TODO
    }

    RetType attach_to_shm() {
        // TODO
    }

    RetType detach_from_shm() {
        // TODO
    }

    RetType destroy_shm() {
        // TODO
    }

    uint8_t* get_shm_block() {
        // TODO
    }

};
