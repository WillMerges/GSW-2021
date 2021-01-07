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
#include <stdint.h>
#include <stdlib.h>
#include "lib/vcm/vcm.h"
#include "common/types.h"

namespace shm {
    // used for generating shm id
    const int id = 65; // random number
    const int info_id = 23;

    // get the size of the block
    size_t get_shmem_size();

    // attach the current process to the shared memory block
    RetType attach_to_shm(vcm::VCM* vcm);

    // detach the current process to the shared memory block
    RetType detach_from_shm();

    // destroy the current shared memory
    RetType destroy_shm();

    // write to shared memory
    // returns failure if not all bytes were able to be written
    RetType write_to_shm(void* src, size_t size, size_t offset = 0);

    // read from shared memory, size is max size to read
    // doesn't care how recent the read was
    // returns failure if not all bytes were able to be read
    RetType read_from_shm(void* dst, size_t size, size_t offset = 0);

    // only reads if there has been a write since the last read, otherwise returns failure
    RetType read_from_shm_if_updated(void* dst, size_t size, size_t offset = 0);

    // reads from shared mem, blocks until there's a write
    // blocking is not a spin lock, process will no longer be scheduled
    RetType read_from_shm_block(void* dst, size_t size, size_t offset = 0);

    // create shared memory
    RetType create_shm(vcm::VCM* vcm);

    // set all shared memory to zero
    RetType clear_shm();
}
