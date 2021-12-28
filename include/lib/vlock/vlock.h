/********************************************************************
*  Name: vlock.h
*
*  Purpose: Header for vehicle locking library, controls locking of
*           physical resources on the vehicle.
*
*  NOTE: Locking is not per-VCM file, it is for the entiriety of GSW.
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#ifndef VLOCK_H
#define VLOCK_H

#include "common/types.h"
#include <semaphore.h>
#include <stdlib.h>

namespace vlock {
    static const int key_id = 0x57; // some unique integer

    typedef enum {
        ENGINE_CONTROLLER,
        NUM_RESOURCES
    } vlock_t;

    // Create vehicle lock shared memory block
    RetType create_shm();

    // Destroy vehicle lock shared memory block
    RetType destroy_shm();

    // Initialize vehicle lock library
    RetType init();

    // Try to lock a vehicle resource
    // NOTE: must call 'init' first
    RetType try_lock(vlock_t resource);

    // Unlock a locked vehicle resource
    // Should only be called if a successful call to 'try_lock' has been made
    // NOTE: must call 'init' first
    RetType unlock(vlock_t resource);

    // Semaphore block
    sem_t* locks = NULL;
}

#endif
