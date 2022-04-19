/*******************************************************************************
* Name: dl_shm.cpp
*
* Purpose: Data Logging Shared Memory
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
#include "lib/dls/dl_shm.h"
#include "lib/dls/dls.h"
#include <semaphore.h>

using namespace dls;
using namespace shm;


typedef struct {
    sem_t lock;
    bool enabled;
} dl_shm_t;

DlShm::DlShm() {
    shm = NULL;
}

DlShm::~DlShm() {
    if(shm) {
        delete shm;
    }
}

RetType DlShm::init() {
    MsgLogger logger("DlShm", "init");

    if(shm) {
        logger.log_message("already initialized");
        return SUCCESS;
    }

    char* env = getenv("GSW_HOME");
    if(env == NULL) {
        logger.log_message("GSW_HOME environment variable not set!");
        return FAILURE;
    }

    // use shared libary location as part of shared memory key
    key_filename = env;
    key_filename += "/";
    key_filename += "lib/bin/libdls.so";

    shm = new Shm(key_filename.c_str(), key_id, sizeof(dl_shm_t));

    return SUCCESS;
}

RetType DlShm::attach() {
    MsgLogger logger("DlShm", "attach");

    if(!shm) {
        logger.log_message("not initialized, cannot attach");
        return FAILURE;
    }

    return shm->attach();
}

RetType DlShm::detach() {
    MsgLogger logger("DlShm", "detach");

    if(!shm) {
        logger.log_message("not initialized, cannot detach");
        return FAILURE;
    }

    return shm->detach();
}

RetType DlShm::create() {
    MsgLogger logger("DlShm", "create");

    if(!shm) {
        logger.log_message("not initialized, cannot create");
        return FAILURE;
    }

    RetType ret = shm->create();

    if(SUCCESS != ret) {
        return ret;
    }

    if(SUCCESS != shm->attach()) {
        logger.log_message("failed to attach, cannot set default value");
        return FAILURE;
    }

    dl_shm_t* data = (dl_shm_t*)shm->data;

    if(0 != sem_init(&(data->lock), 1, 1)) {
        logger.log_message("failed to initialize semaphore");
        return FAILURE;
    }

    data->enabled = true;

    if(SUCCESS != shm->detach()) {
        logger.log_message("failed to detach");
        return FAILURE;
    }

    return SUCCESS;
}

RetType DlShm::destroy() {
    MsgLogger logger("DlShm", "destroy");

    if(!shm) {
        logger.log_message("not initialized, cannot destroy");
        return FAILURE;
    }

    return shm->destroy();
}

RetType DlShm::logging_enabled(bool* enabled) {
    MsgLogger logger("DlShm", "logging_enabled");

    if(!shm) {
        logger.log_message("not initialized");
        return FAILURE;
    }

    dl_shm_t* data = (dl_shm_t*)(shm->data);

    if(0 != sem_wait(&(data->lock))) {
        logger.log_message("semaphore wait failed");
        return FAILURE;
    }

    *enabled = data->enabled;

    if(0 != sem_post(&(data->lock))) {
        logger.log_message("semaphore post failed");
        return FAILURE;
    }

    return SUCCESS;
}

RetType DlShm::set_logging(bool enabled) {
    MsgLogger logger("DlShm", "set_logging");

    if(!shm) {
        logger.log_message("not initialized");
        return FAILURE;
    }

    dl_shm_t* data = (dl_shm_t*)(shm->data);

    if(0 != sem_wait(&(data->lock))) {
        logger.log_message("semaphore wait failed");
        return FAILURE;
    }

    data->enabled = enabled;

    if(0 != sem_post(&(data->lock))) {
        logger.log_message("semaphore post failed");
        return FAILURE;
    }

    return SUCCESS;
}
