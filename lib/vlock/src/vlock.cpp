/********************************************************************
*  Name: vlock.cpp
*
*  Purpose: Vehicle locking library, controls locking of
*           physical resources on the vehicle.
*
*  NOTE: Locking is not per-VCM file, it is for the entiriety of GSW.
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include "lib/vlock/vlock.h"
#include "lib/dls/dls.h"
#include "lib/shm/shm.h"
#include <string>

using namespace dls;
using namespace shm;

// initialize semaphore macro
#define INIT(X, V) \
    if(0 != sem_init( &( (X) ), 1, (V) )) { \
        return FAILURE; \
    } \


RetType vlock::create_shm() {
    MsgLogger logger("VLOCK", "create_shm");

    char* env = getenv("GSW_HOME");
    if(env == NULL) {
        logger.log_message("GSW_HOME environment variable not set!");
        return FAILURE;
    }

    // use shared libary location as part of shared memory key
    std::string key_filename = env;
    key_filename += "/";
    key_filename += "lib/bin/libvlock.so";

    Shm shm(key_filename.c_str(), key_id, sizeof(sem_t) * NUM_RESOURCES);

    if(SUCCESS != shm.create()) {
        logger.log_message("Failed to create shared memory");
        return FAILURE;
    }

    // initialize semaphores
    if(SUCCESS != shm.attach()) {
        logger.log_message("Failed to attach to shared memory, cannot initialize semaphores");
        return FAILURE;
    }

    locks = (sem_t*)(shm.data);
    for(int i = 0; i < NUM_RESOURCES; i++) {
        INIT(locks[i], 1);
    }

    return SUCCESS;
}

RetType vlock::destroy_shm() {
    MsgLogger logger("VLOCK", "destroy_shm");

    char* env = getenv("GSW_HOME");
    if(env == NULL) {
        logger.log_message("GSW_HOME environment variable not set!");
        return FAILURE;
    }

    // use shared libary location as part of shared memory key
    std::string key_filename = env;
    key_filename += "/";
    key_filename += "lib/bin/libvlock.so";

    Shm shm(key_filename.c_str(), key_id, sizeof(sem_t) * NUM_RESOURCES);

    if(SUCCESS != shm.attach()) {
        logger.log_message("Failed to attach to shared memory, cannot destroy");
        return FAILURE;
    }

    if(SUCCESS != shm.destroy()) {
        logger.log_message("Failed to destroy shared memory");
        return FAILURE;
    }

    return SUCCESS;
}

RetType vlock::init() {
    MsgLogger logger("VLOCK", "init");

    char* env = getenv("GSW_HOME");
    if(env == NULL) {
        logger.log_message("GSW_HOME environment variable not set!");
        return FAILURE;
    }

    // use shared libary location as part of shared memory key
    std::string key_filename = env;
    key_filename += "/";
    key_filename += "lib/bin/libvlock.so";

    Shm shm(key_filename.c_str(), key_id, sizeof(sem_t) * NUM_RESOURCES);

    if(SUCCESS != shm.attach()) {
        logger.log_message("Failed to create shared memory");
        return FAILURE;
    }

    locks = (sem_t*)(shm.data);

    return SUCCESS;
}

RetType vlock::try_lock(vlock_t resource) {
    MsgLogger logger("VLOCK", "try_lock");

    if(!locks) {
        logger.log_message("Shared memory not attached, cannot lock");
        return FAILURE;
    }

    if(resource >= NUM_RESOURCES) {
        logger.log_message("Invalid resource, cannot lock");
        return FAILURE;
    }

    if(0 != sem_trywait(&(locks[resource]))) {
        // either locked or error
        return FAILURE;
    }

    return SUCCESS;
}

RetType vlock::unlock(vlock_t resource) {
    MsgLogger logger("VLOCK", "unlock");

    if(!locks) {
        logger.log_message("Shared memory not attached, cannot unlock");
        return FAILURE;
    }

    if(resource >= NUM_RESOURCES) {
        logger.log_message("Invalid resource, cannot unlock");
        return FAILURE;
    }

    if(0 != sem_post(&(locks[resource]))) {
        // either locked or error
        return FAILURE;
    }

    return SUCCESS;
}

#undef INIT
