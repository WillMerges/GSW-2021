#include "lib/clock/clock.h"
#include "lib/dls/dls.h"
#include "common/time.h"
#include <string.h>

using namespace countdown_clock;
using namespace dls;

CountdownClock::CountdownClock() {
    shm = NULL;
}

CountdownClock::~CountdownClock() {
    if(shm) {
        delete shm;
    }
}

RetType CountdownClock::init() {
    MsgLogger logger("CountdownClock", "init");


    char* env = getenv("GSW_HOME");
    if(env == NULL) {
        logger.log_message("GSW_HOME environment variable not set!");
        return FAILURE;
    }

    // use shared libary location as part of shared memory key
    std::string key_filename = env;
    key_filename += "/";
    key_filename += "lib/bin/libclock.so";

    shm = new Shm(key_filename.c_str(), shm_key_id, sizeof(clock_shm_t));

    return SUCCESS;
}

RetType CountdownClock::create() {
    MsgLogger logger("CountdownClock", "create");

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
    memset((void*)shm->data, 0, sizeof(clock_shm_t));

    // initialize the semaphore
    clock_shm_t* data = (clock_shm_t*)shm->data;
    if(0 != sem_init(&(data->sem), 1, 1)) {
        logger.log_message("Failed to initialize semaphore");
        return FAILURE;
    }

    return SUCCESS;
}

RetType CountdownClock::destroy() {
    return shm->destroy();
}

RetType CountdownClock::open() {
    return shm->attach();
}

RetType CountdownClock::close() {
    return shm->detach();
}

RetType CountdownClock::parse_cmd(clock_cmd_t* cmd) {
    MsgLogger logger("CountdownClock", "parse_cmd");

    RetType ret = SUCCESS;

    clock_shm_t* data = (clock_shm_t*)shm->data;
    if(data == NULL) {
        logger.log_message("Shared memory not attached, invalid address");
        return FAILURE;
    }

    // lock shared memory
    if(0 != sem_wait(&(data->sem))) {
        logger.log_message("Error locking semaphore");
        return FAILURE;
    }

    uint32_t curr_time = systime();

    // handle command
    switch(cmd->cmd) {
        case START_CLOCK:
            data->stopped = false;
            break;
        case STOP_CLOCK:
            if(!data->stopped) {
                data->stop_time = curr_time;
                data->stopped = true;
            }
            break;
        case HOLD_CLOCK:
            data->hold = curr_time + cmd->arg;
            break;
        case SET_CLOCK:
            data->t0 = curr_time + cmd->arg;
            break;
    }

    // unlock shared memory
    if(0 != sem_post(&(data->sem))) {
        logger.log_message("Error unlocking semaphore");
        return FAILURE;
    }

    return ret;
}

RetType CountdownClock::read_time(int64_t* time) {
    MsgLogger logger("CountdownClock", "read_time");

    clock_shm_t* data = (clock_shm_t*)shm->data;
    if(data == NULL) {
        logger.log_message("Shared memory not attached, invalid address");
        return FAILURE;
    }

    // lock shared memory
    if(0 != sem_wait(&(data->sem))) {
        logger.log_message("Error locking semaphore");
        return FAILURE;
    }

    uint32_t curr_time = systime();

    if(data->stopped) {
        *time = ((int64_t)curr_time) - ((int64_t)data->stop_time);
    } else {
        if(curr_time > data->hold) {
            *time = ((int64_t)data->t0) - ((int64_t)data->hold);
        } else {
            *time = ((int64_t)curr_time) - ((int64_t)data->t0);
        }
    }

    // unlock shared memory
    if(0 != sem_post(&(data->sem))) {
        logger.log_message("Error unlocking semaphore");
        return FAILURE;
    }

    return SUCCESS;
}

// RetType CountdownClock::block_until(int64_t time) {
// TODO
// }
