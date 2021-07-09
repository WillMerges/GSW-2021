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
#include "lib/vcm/vcm.h"
#include "common/types.h"

/*
* Telemetry shared memory is per-vehicle
* Each vehicle can have multiple telemetry packets that are updated at varying rates
* Each telemetry packet will get it's own shared memory block to be stored in
* Each telemetry block will have a corresponding info shared memory block to facilitate access control
* There will also be a 'master block' containing locking information for the whole vehicle
*/

class TelemetryShm {
public:
    // constructor
    Shm(vcm::VCM vcm);

    // destructor
    virtual ~Shm();

    // attach to all shared memory blocks corresponding to a vehicle
    // shared memory blocks must already exist, this will simply attach to them
    RetType init();

    // open shared memory
    RetType open();

    // close shared memory
    RetType close();

    // create all shared memory for a vehicle
    RetType create();

    // destroy all shared memory for a vehicle
    RetType destroy();

    // write 'len' bytes from 'data' to telemetry block number 'packet_id'
    // if any bytes fail to write FAILURE is returned
    // NOTE: this is a blocking operation
    RetType write(unsigned int packet_id, uint8_t* data, size_t len);

    // clear telemetry block corresponding to 'packet_id' with value 'val'
    // returns FAULURE if any bytes fail to clear
    // NOTE: this is a blocking operation
    RetType clear(unsigned int packet_id, uint8_t val = 0x0);

    // read 'len' bytes into 'buff' from telemetry block number 'packet_id'
    // the number of bytes actually read is placed into 'read'
    // different read modes are able to be set with set_read_mode
    // if any bytes fail to read FAILURE is returned
    // read may return BLOCKED depending on the read mode
    RetType read(unsigned int packet_id, uint8_t buff, size_t len);

    // reading modes
    typedef enum {
        STANDARD_READ,   // read no matter what, regardless if the data updated since the last read
        BLOCKING_READ,   // blocks until new data has been written, then reads
        NONBLOCKING_READ // returns BLOCKED if the data has not updated since the last read
    } read_mode_t;

    // set the reading mode
    void set_read_mode(read_mode_t mode);

private:
    bool open;

    // TODO other stuff
};

#endif
