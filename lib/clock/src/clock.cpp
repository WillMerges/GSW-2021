/*******************************************************************************
* Name: clock.cpp
*
* Purpose: Countdown clock access and control library
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
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
    key_filename = env;
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

    clock_shm_t* data = (clock_shm_t*)shm->data;

    // set the hold time and t0 to right now
    uint32_t curr_time = systime();
    data->t0 = data->stop_time = curr_time;
    data->stopped = true;

    // initialize the semaphore
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

    uint32_t curr_time = systime();

    // lock shared memory
    if(0 != sem_wait(&(data->sem))) {
        logger.log_message("Error locking semaphore");
        return FAILURE;
    }

    int64_t ref_t0;

    // handle command
    switch(cmd->cmd) {
        case START_CLOCK:
            if(data->stopped) {
                // set a new t0 reference time if we're coming out of a stop
                data->stopped = false;
                data->t0 = curr_time + (((int64_t)data->t0) - ((int64_t)data->stop_time));

                if(data->hold_set) {
                    // if we're already past a hold, unset it
                    if(curr_time > data->t0 + data->hold) {
                        data->hold_set = false;
                    }
                }
            }
            break;
        case STOP_CLOCK:
            if(!data->stopped) {
                data->stop_time = curr_time;
                data->stopped = true;

                // if we're at a hold, adjust t0 forward so the stop lines up
                if(data->hold_set && curr_time >= data->t0 + data->hold) {
                    data->t0 = data->stop_time - data->hold;
                }
            }
            break;
        case SET_HOLD_CLOCK:
            ref_t0 = data->t0;
            if(data->stopped) {
                ref_t0 = curr_time + (((int64_t)data->t0) - ((int64_t)data->stop_time));
            } else if(data->hold_set && curr_time > data->t0 - data->hold) {
                logger.log_message("already at a hold, must release first!");
                printf("already at a hold, must release first!\n");

                ret = FAILURE;
                break;
            }

            if(curr_time > ref_t0 + cmd->arg) {
                // this hold has already passed, we can't set it
                logger.log_message("Hold time has already passed, cannot set");
                printf("Hold time has already passed, cannot set\n");

                ret = FAILURE;
            } else {
                // set the hold
                data->hold = cmd->arg;
                data->hold_set = true;
            }
            break;
        case RELEASE_HOLD_CLOCK:
            if(data->hold_set) {
                data->hold_set = false;

                // set the new t0 time if we are past the hold
                if(curr_time >= data->t0 + data->hold) {
                    ref_t0 = data->t0;
                    data->t0 = curr_time - data->hold;

                    // if we are stopped, adjust the stop time
                    if(data->stopped) {
                        data->stop_time = data->stop_time + (data->t0 - ref_t0);
                    }
                }
            }
            break;
        case SET_CLOCK:
            // set a new t0
            data->t0 = ((int64_t)curr_time) - cmd->arg;

            // if we were stopped, set a new reference time
            if(data->stopped) {
                data->stop_time = curr_time;
            }
            break;
        case NUM_CLOCK_CMDS:
            // do nothing
            break;
    }

    // unlock shared memory
    if(0 != sem_post(&(data->sem))) {
        logger.log_message("Error unlocking semaphore");
        return FAILURE;
    }

    return ret;
}

RetType CountdownClock::read_time(int64_t* time, bool* stopped, int64_t* hold_time, bool* hold_set) {
    MsgLogger logger("CountdownClock", "read_time");

    clock_shm_t* data = (clock_shm_t*)shm->data;
    if(data == NULL) {
        logger.log_message("Shared memory not attached, invalid address");
        return FAILURE;
    }

    uint32_t curr_time = systime();

    // lock shared memory
    if(0 != sem_wait(&(data->sem))) {
        logger.log_message("Error locking semaphore");
        return FAILURE;
    }

    if(hold_time && hold_set) {
        *hold_set = data->hold_set;
        if(data->hold_set) {
            *hold_time = ((int64_t)data->hold);
        }
    }

    *stopped = data->stopped;

    if(data->stopped) {
        *time = ((int64_t)data->stop_time) - ((int64_t)data->t0) ;
    } else {
        if(data->hold_set && curr_time > (data->t0 + data->hold)) {
            *time = (int64_t)(data->hold);
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
//     // TODO
// }

void CountdownClock::to_str(int64_t& time, std::string* str) {
    std::string t = "T";
    if(time > 0) {
        t += "+";
    } else {
        t += "-";
        time *= -1;
    }

    // TODO precompute some of these constants
    unsigned int ms_per_hour = (60 * 60 * 1000);
    std::string hh = std::to_string(time / ms_per_hour);
    if(hh.size() < 2) {
        hh = "0" + hh;
    }
    time %= ms_per_hour;

    unsigned int ms_per_minute = (60 * 1000);
    std::string mm = std::to_string(time / ms_per_minute);
    if(mm.size() < 2) {
        mm = "0" + mm;
    }
    time %= ms_per_minute;

    unsigned int ms_per_second = 1000;
    std::string ss = std::to_string(time / ms_per_second);
    if(ss.size() < 2) {
        ss = "0" + ss;
    }
    time %= ms_per_second;

    std::string ms = std::to_string(time);

    t = t + " " + hh + ":" + mm + ":" + ss + "." + ms;
    *str = t;
}
