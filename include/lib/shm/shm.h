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
#include <semaphore.h>
#include "common/types.h"

namespace shm {

    // faciliates access to shared memory
    class SHM {
    public:
        // constructor
        // 'file' is a unique file location to use as a shared memory key
        // 'size' is the size in bytes of the shared memory block
        SHM(std::string file, size_t size);

        // default destructor
        virtual ~SHM() {}

        // get the size of the shared memory block
        size_t get_size();

        // attach the current process to the shared memory block
        RetType attach_to_shm();

        // detach the current process from the shared memory block
        RetType detach_from_shm();

        // destroy the current shared memory
        // NOTE: must have called create or attach first
        RetType destroy_shm();

        // write to shared memory
        // returns failure if not all bytes were able to be written
        RetType write_to_shm(void* src, size_t size, uint32_t write_id = 0, size_t offset = 0);

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
        RetType create_shm();

        // set all shared memory to 'value'
        RetType clear_shm(uint8_t val = 0x0, uint32_t write_id = 0);

    private:
        // used for generating shm id
        static const int id = 65; // random number
        static const int info_id = 23;

        // info block for locking main shared memory
        typedef struct {
            uint32_t nonce; // nonce that updates every write
            uint32_t write_id; // unique identifier for 'who' did the writing
            unsigned int readers;
            unsigned int writers;
            sem_t rmutex;
            sem_t wmutex;
            sem_t readTry;
            sem_t resource;
        } shm_info_t;

        unsigned int last_nonce;

        // info shared memory block
        shm_info_t* info;
        int info_shmid;

        // main shared memory block
        void* shmem;
        int shmid;

        // size of shared memory
        size_t shm_size;

        // unique file location to use as key of the shared memory block
        std::string file_key;

    };

}

#endif
