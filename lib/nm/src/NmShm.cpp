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
}

RetType NmShm::init() {
    MsgLogger logger("NmShm", "init");

    char* env = getenv("GSW_HOME");
    if(env == NULL) {
        logger.log_message("GSW_HOME environment variable not set!");
        return FAILURE;
    }

    // use shared libary location as part of shared memory key
    key_filename = env;
    key_filename += "/";
    key_filename += "lib/bin/libnm.so";

    last_nonce = 0;

    shm = new Shm(key_filename.c_str(), key_id, sizeof(nm_shm_t));

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
    memset((void*)shm->data, 0, sizeof(nm_shm_t));

    // initialize the semaphore
    nm_shm_t* data = (nm_shm_t*)shm->data;
    if(0 != sem_init(&(data->sem), 1, 1)) {
        logger.log_message("Failed to initialize semaphore");
        return FAILURE;
    }

    data->nonce = 0;

    return SUCCESS;
}

RetType NmShm::destroy() {
    return shm->destroy();
}

RetType NmShm::get_addr(struct sockaddr_in* addr) {
    MsgLogger logger("NmShm", "get_addr");

    static unsigned int blocked = block_count; // make sure we block the first time so we get whatever address we have

    nm_shm_t* data = (nm_shm_t*)shm->data;
    if(data == NULL) {
        logger.log_message("Shared memory not attached, invalid address");
        return FAILURE;
    }

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

    if(data->nonce == last_nonce) {
        last_nonce = data->nonce;
        return NOCHANGE;
    }

    last_nonce = data->nonce;

    // copy data out
    *addr = data->addr;

    // unlock shared memory
    if(0 != sem_post(&(data->sem))) {
        logger.log_message("Error unlocking semaphore");
        return FAILURE;
    }

    // reset blocked counter
    blocked = 0;

    return SUCCESS;
}

RetType NmShm::update(struct sockaddr_in* addr) {
    MsgLogger logger("NmShm", "update");

    nm_shm_t* data = (nm_shm_t*)shm->data;
    if(data == NULL) {
        logger.log_message("Shared memory not attached, invalid address");
        return FAILURE;
    }

    // lock shared memory
    if(0 != sem_wait(&(data->sem))) {
        logger.log_message("Error locking semaphore");
        return FAILURE;
    }

    // copy data in
    data->addr = *addr;

    // unlock shared memory
    if(0 != sem_post(&(data->sem))) {
        logger.log_message("Error unlocking semaphore");
        return FAILURE;
    }

    return SUCCESS;
}
