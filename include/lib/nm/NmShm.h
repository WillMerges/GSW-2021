#ifndef NM_SHH_H
#define NM_SHM_H

#include <semaphore.h>
#include <netinet/in.h>
#include <string>
#include "lib/shm/shm.h"
#include "common/types.h"

using namespace shm;

/*
Network Manager shared memory
Holds network layer address (IPv4 for now) of receiver

Intended operation is for processes that receive telemetry to strip the IP
from the packet and write it to shared memory so the sending process knows
where to send packets to.
*/

class NmShm {
public:
    // constructor
    // block count is how many times 'get_addr' can fail to lock shared memory before it forces a block
    NmShm(unsigned int block_count = 10);

    // destructor
    virtual ~NmShm();

    // initialize the object
    RetType init(size_t num_devices);

    // attach to shared memory
    RetType attach();

    // detach from shared memory
    RetType detach();

    // create shared memory
    RetType create();

    // destroy shared memory
    RetType destroy();

    // Get the address stored in shared memory
    // If shared memory is currently locked, we just unblock, unless we've already blocked for 'block_count' many times
    // first call to 'get_addr' always blocks
    RetType get_addr(uint32_t device_id, struct sockaddr_in* addr);

    // Updated shared memory with this address
    // if shared memory is locked, returns immediately unless we've already blocked for 'block_count' many time
    // first call to 'update_addr' always blocks and guarantees data is copied in
    RetType update_addr(uint32_t device_id, struct sockaddr_in* addr);

private:
    // random number used to generate shared memory token
    static const int key_id = 0x54;

    // shared memory object
    Shm* shm;

    // layout of Network Manager shared memory
    typedef struct {
        sem_t sem;
        uint32_t nonce;
        struct sockaddr_in addr;
    } nm_shm_t;

    // last nonce we've read for each device
    uint32_t* last_nonces;

    // max times before we block on get_addr
    unsigned int block_count;

    // file to use for generating shared memory token
    std::string key_filename;

    // number of devices we're tracking
    size_t num_devices;
};

#endif
