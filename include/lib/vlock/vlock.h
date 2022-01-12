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
#include <stdint.h>

namespace vlock {
    static const int key_id = 0x57; // some unique integer

    typedef enum {
        ENGINE_CONTROLLER_PROGRAM,
        ENGINE_CONTROLLER_COMMAND,
        NUM_RESOURCES
    } vlock_t;

    // Create vehicle lock shared memory block
    RetType create_shm();

    // Destroy vehicle lock shared memory block
    RetType destroy_shm();

    // Initialize vehicle lock library
    RetType init();

    // Try to lock a vehicle resource
    // returns SUCCESS if locked successfully, LOCKED if unable to obtain lock,
    // and FAILURE otherwise
    // NOTE: must call 'init' first
    RetType try_lock(vlock_t resource);

    // Try to lock a vehicle resource
    // wait for 'wait' milliseconds before returning TIMEOUT
    // if 'wait' is 0, waits indefinitely for the resource
    RetType lock(vlock_t resouce, uint32_t wait);

    // Unlock a locked vehicle resource
    // Should only be called if a successful call to 'try_lock' or 'lock' has been made
    // returns SUCCESS on success, FAILURE otherwise
    // NOTE: must call 'init' first
    RetType unlock(vlock_t resource);

    // Semaphore block
    sem_t* locks = NULL;
}

#endif
