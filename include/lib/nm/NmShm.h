#ifndef NM_SHH_H
#define NM_SHM_H

#include <semaphore.h>
#include "lib/shm/shm.h"
#include "common/types.h"

using namespace shm;

/*
Network Manager shared memory
Holds network layer address (IPv4 for now) of receiver

Intended operation is for processes that receive telemetry to strip the IP
from the packet and write it to shared memory so the sending process knows
where to send packets to.

Uses a single lock, if it's currently locked and a process tries to write, don't
block or wait for the lock to free, just don't write that address to shared
memory. All receiving processes for a vehicle should be receiving from the same
address (TODO if this is not true anymore, this will need to hold multiple IPs).
Since they all receive from the same address, it's not a big deal if we miss a
write. Even if the address changes during operation, we don't care if we have to
wait for one more packet to be able to send to the correct address again since
changing IPs of the receiver is not a normal circumstance.
*/

class NmShm {
public:
    // constructor
    NmShm();

    // destructor
    virtual ~NmShm();

    // initialize the object using 'vcm'
    // NOTE: shared memory must already be created first, one process must call
    //       'create' before calling init.
    RetType init(VCM* vcm);

    // initialize the object using the default vcm config file
    // NOTE: shared memory must already be created first, one process must call
    //       'create' before calling init.
    RetType init();

    // create shared memory
    static RetType create();

    // destroy shared memory
    static RetType destroy();
};

#endif
