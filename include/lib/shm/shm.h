/********************************************************************
*  Name: shm.h
*
*  Purpose: Header for utility functions for creating/modifying
*           shared memory.
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include "common/types.h"
#include <stdlib.h>

namespace shm {
    // used for generating shm id
    const int id = 65;
    const char* const file = "/gsw/shmfile";

    // set the size of the block
    void set_size(size_t size);

    // attach the current process to the shared memory block
    RetType attach_to_shm();

    // detach the current process to the shared memory block
    RetType detach_from_shm();

    // destroy the current shared memory
    RetType destroy_shm();

    // return pointer to shared memory block
    void* get_shm_block();
}
