#include "lib/nm/NmShm.h"
#include "lib/dls/dls.h"

#include <string.h>

using namespace dls;
using namespace shm;

NmShm::NmShm(unsigned int block_count) {
    shm = NULL;
    this->block_count = block_count;
}

NmShm::~NmShm() {
    if(shm) {
        delete shm;
    }

    if(last_nonces) {
        delete[] last_nonces;
    }
}

RetType NmShm::init(size_t num_devices) {
    MsgLogger logger("NmShm", "init");

    this->num_devices = num_devices;

    char* env = getenv("GSW_HOME");
    if(env == NULL) {
        logger.log_message("GSW_HOME environment variable not set!");
        return FAILURE;
    }

    // use shared libary location as part of shared memory key
    key_filename = env;
    key_filename += "/";
    key_filename += "lib/bin/libnm.so";

    last_nonces = new uint32_t[num_devices];
    memset((void*)last_nonces, 0, sizeof(uint32_t) * num_devices);

    shm = new Shm(key_filename.c_str(), key_id, num_devices * sizeof(nm_shm_t));

    return SUCCESS;
}

RetType NmShm::attach() {
    return shm->attach();
}

RetType NmShm::detach() {
    return shm->detach();
}

RetType NmShm::create() {
    MsgLogger logger("NmShm", "create");

    // create shared memory
    if(shm->create() != SUCCESS) {
        logger.log_message("Unable to create shared memory");
        return FAILURE;
    }

    if(shm->attach() != SUCCESS) {
        logger.log_message("Unable to attach to shared memory");
        return FAILURE;
    }

    // zero shared memory
    memset((void*)(shm->data), 0, num_devices * sizeof(nm_shm_t));

    // initialize the semaphores
    nm_shm_t* data = ((nm_shm_t*)shm->data);

    for(size_t i = 0; i < num_devices; i++) {
        if(0 != sem_init(&(data[i].sem), 1, 1)) {
            logger.log_message("Failed to initialize semaphore");
            return FAILURE;
        }

        // zero nonce
        data[i].nonce = 0;
    }


    return SUCCESS;
}

RetType NmShm::destroy() {
    return shm->destroy();
}

RetType NmShm::get_addr(uint32_t device_id, struct sockaddr_in* addr) {
    MsgLogger logger("NmShm", "get_addr");
    RetType ret = SUCCESS;

    static unsigned int blocked = block_count; // make sure we block the first time so we get whatever address we have

    if(shm->data == NULL) {
        logger.log_message("Shared memory not attached, invalid address");
        return FAILURE;
    }

    nm_shm_t* data = ((nm_shm_t*)shm->data) + device_id;

    // lock shared memory
    if(blocked < block_count) {
        // non-blocking
        if(0 != sem_trywait(&(data->sem))) {
            if(EAGAIN != errno) {
                logger.log_message("Error locking semaphore");
                return FAILURE;
            } else {
                blocked++;
                return BLOCKED;
            }
        }
    } else {
        // blocking
        if(0 != sem_wait(&(data->sem))) {
            logger.log_message("Error locking semaphore");
            return FAILURE;
        }
    }

    if(data->nonce != last_nonces[device_id]) {
        last_nonces[device_id] = data->nonce;

        // copy data out
        *addr = data->addr;
    } else {
        ret = NOCHANGE;
    }

    // unlock shared memory
    if(0 != sem_post(&(data->sem))) {
        logger.log_message("Error unlocking semaphore");
        return FAILURE;
    }

    // reset blocked counter
    blocked = 0;

    return ret;
}

RetType NmShm::update_addr(uint32_t device_id, struct sockaddr_in* addr) {
    MsgLogger logger("NmShm", "update_addr");

    static unsigned int blocked = block_count; // make sure we block the first time so we get whatever address we have

    if(shm->data == NULL) {
        logger.log_message("Shared memory not attached, invalid address");
        return FAILURE;
    }

    if(device_id >= num_devices) {
        logger.log_message("Invalid device ID");
        return FAILURE;
    }

    nm_shm_t* data = ((nm_shm_t*)shm->data) + device_id;

    // lock shared memory
    if(blocked < block_count) {
        // non-blocking
        if(0 != sem_trywait(&(data->sem))) {
            if(EAGAIN != errno) {
                logger.log_message("Error locking semaphore");
                return FAILURE;
            } else {
                blocked++;
                return BLOCKED;
            }
        }
    } else {
        // blocking
        if(0 != sem_wait(&(data->sem))) {
            logger.log_message("Error locking semaphore");
            return FAILURE;
        }
    }

    // copy data in
    data->addr = *addr;
    data->nonce++;

    // unlock shared memory
    if(0 != sem_post(&(data->sem))) {
        logger.log_message("Error unlocking semaphore");
        return FAILURE;
    }

    // reset blocked counter
    blocked = 0;

    return SUCCESS;
}
