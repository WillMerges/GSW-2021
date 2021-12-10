/*******************************************************************************
* Name: clock.h
*
* Purpose: Countdown clock access and control library
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
#ifndef CLOCK_H
#define CLOCK

#include "lib/vcm/vcm.h"
#include "lib/shm/shm.h"
#include "common/types.h"
#include <stdint.h>
#include <semaphore.h>
#include <string>
#include <time.h>

using namespace shm;
using namespace vcm;

namespace countdown_clock {

    typedef enum {
        START_CLOCK,
        STOP_CLOCK,
        SET_HOLD_CLOCK,
        RELEASE_HOLD_CLOCK,
        SET_CLOCK
    } cmd_type;

    typedef struct {
        cmd_type cmd;
        int64_t arg; // ms (- is before launch, + is after)
    } clock_cmd_t;

    // Represents a clock that exists in shared memory
    class CountdownClock {
    public:
        // Constructor
        CountdownClock();

        // Desctructor
        virtual ~CountdownClock();

        // Initialize the clock object, needs to be done before using any function
        RetType init();

        // Create shared memory
        RetType create();

        // Delete shared memory
        RetType destroy();

        // Open shared memory
        RetType open();

        // Close shared memory
        RetType close();

        // Parse a clock command
        RetType parse_cmd(clock_cmd_t* cmd);

        // Read the current time
        // NOTE: blocking function
        RetType read_time(int64_t* time);

        // Block until a certain time
        // NOTE: blocking function
        // RetType block_until(int64_t time);

        // Convert a int64 time to a string
        // outputed as T+/- hh:mm:ss.ms
        // RetType to_str(int64_t time, std::string* str);

    private:
        typedef struct {
            sem_t sem;
            uint32_t t0;        // t-0 time (ms)
            bool hold_set;      // if a hold was set
            uint32_t hold;      // hold time (ms)
            bool stopped;       // if the clock is stopped
            uint32_t stop_time; // time the clock was stopped
        } clock_shm_t;

        static const int shm_key_id = 0x92; // random unique int for shared memory key
        static const clockid_t clock_id = CLOCK_REALTIME;

        Shm* shm;
    };
}

#endif
