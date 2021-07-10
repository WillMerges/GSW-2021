/*******************************************************************************
* Name: TelemetryShm.cpp
*
* Purpose: Telemetry shared memory access and control for readers and writers
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
#ifndef TELSHM_H
#define TELSHM_H

#include <stdint.h>
#include <stdlib.h>
#include <semaphore.h>
#include "lib/vcm/vcm.h"
#include "lib/shm/shm.h"
#include "common/types.h"

/*
* Telemetry shared memory is per-vehicle
* Each vehicle can have multiple telemetry packets that are updated at varying rates
* Each telemetry packet will get it's own shared memory block to be stored in
* Each telemetry block will have a corresponding info shared memory block to facilitate access control
* There will also be a 'master block' containing locking information for the whole vehicle
*/

using namespace shm;
using namespace vcm;

class TelemetryShm {
public:
    // constructor
    TelemetryShm();

    // destructor
    virtual ~TelemetryShm();

    // initialize the object using 'vcm'
    RetType init(VCM vcm);

    // initialize the object using the default vcm config file
    RetType init();

    // open shared memory
    RetType open();

    // close shared memory
    RetType close();

    // create all shared memory for a vehicle
    // NOTE: does not attach!
    RetType create();

    // destroy all shared memory for a vehicle
    // NOTE: must be attached already!
    RetType destroy();

    // write the packet size bytes from 'data' to telemetry block number 'packet_id'
    // does not do any size check, data must be at least as large as the packet size
    // if any bytes fail to write FAILURE is returned
    // NOTE: this is a blocking operation
    RetType write(unsigned int packet_id, uint8_t* data);

    // clear telemetry block corresponding to 'packet_id' with value 'val'
    // returns FAULURE if any bytes fail to clear
    // NOTE: this is a blocking operation
    RetType clear(unsigned int packet_id, uint8_t val = 0x0);

    // lock the shared memory for a packet as a reader (e.g. dont allow any writers)
    // different read modes are able to be set with set_read_mode
    // if any bytes fail to read FAILURE is returned
    // read may return BLOCKED depending on the read mode
    RetType read_lock(unsigned int packet_id);

    // unlock the shared memory for a packet as a writer
    RetType read_unlock(unsigned int packet_id);

    // reading modes
    typedef enum {
        STANDARD_READ,   // read no matter what, regardless if the data updated since the last read
        BLOCKING_READ,   // blocks until new data has been written, then reads
        NONBLOCKING_READ // returns BLOCKED if the data has not updated since the last read
    } read_mode_t;

    // set the reading mode
    void set_read_mode(read_mode_t mode);

private:
    // info block for locking main shared memory
    typedef struct {
        uint32_t nonce; // nonce that updates every write
        uint32_t readers;
        uint32_t writers;
        sem_t rmutex;
        sem_t wmutex;
        sem_t readTry;
        sem_t resource;
    } shm_info_t;

    read_mode_t read_mode;

    size_t num_packets;
    uint32_t* last_nonces;
    Shm** packet_blocks;
    Shm** info_blocks;
    Shm* master_block; // holds a single block
};

#endif
