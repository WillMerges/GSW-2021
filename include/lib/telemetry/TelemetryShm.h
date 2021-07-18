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
#include <semaphore.h>
#include "lib/vcm/vcm.h"
#include "lib/shm/shm.h"
#include "common/types.h"

/*
* Telemetry shared memory is per-vehicle
* Each vehicle can have multiple telemetry packets stored in shared memory
* Each telemetry packet will get it's own shared memory block to be stored in
* Each telemetry block will have a corresponding nonce stored in shared memory
* There is also be a 'master block' containing locking information for the whole vehicle
*
* Currently locking is vehicle-wide
* Regardless of which packet(s) are being read/written there is one lock
* e.g. even if two writers are writing to different packets only one can write at a time
*
* The above limitation can be solved by storing an entire info block for each
* packet rather than just a nonce. To block on multiple packets would require
* creating a thread for each packet and blocking on that packets nonce from the
* info block. The overhead for creating and cleaning up several threads is likely
* too high to make this method better than sharing a single lock for a small number
* of packets. The master nonce could still be updated and used as a way to block
* until any packet has changed.
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

    // lock the shared memory for packets as a reader (e.g. dont allow any writers)
    // pass packets to read as a list of 'num' packet ids
    // if read mode is set to STANDARD_READ, returns regardless the data changed since the last read
    // if read mode is set to BLOCKING_READ, if the data has not changed since the last read the process will sleep until it changes
    // if read mode is set to NONBLOCKING_READ, returns BLOCKED if the data has not changed since the last read
    RetType read_lock(unsigned int* packet_ids, size_t num);

    // lock all shared memory for all packets
    // functionally no different from other read_lock for STANDARD_READ mode
    // BLOCKING_READ and NONBLOCKING_READ work identically assuming all packets are to be locked
    RetType read_lock();

    // unlock the shared memory, not packet specific
    RetType read_unlock();

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
    uint32_t last_nonce; // last master nonce
    uint32_t* last_nonces; // list of previous nonces for all packets
    Shm** packet_blocks; // list of blocks holding raw telemetry data
    Shm** info_blocks; // list of blocks holding uint32_t nonces
    Shm* master_block; // holds a single shm_info_t for locking
};

#endif
