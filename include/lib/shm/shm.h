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
#ifndef SHM_H
#define SHM_H

#include <stdint.h>
#include <stdlib.h>
#include "common/types.h"

namespace shm {

    // faciliates access to shared memory
    class Shm {
    public:
        // constructor
        // 'file' and 'id' make up a unique key
        // 'size' is the size in bytes of the shared memory block
        Shm(const char* file, const int id, size_t size);

        // default destructor
        virtual ~Shm() {}

        // attach the current process to the shared memory block
        RetType attach();

        // detach the current process from the shared memory block
        RetType detach();

        // destroy the current shared memory block
        // NOTE: must have called create or attach first
        RetType destroy();

        // create shared memory block
        RetType create();

        // pointer to shared memory block
        uint8_t* data;

        // size of shared memory block
        const size_t size;

    private:
        // key values
        const char* key_file;
        const int key_id;

        // shared memory id
        int shmid;
    };
}

#endif
