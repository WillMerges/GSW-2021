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

using namespace shm;
using namespace vcm;

namespace countdown_clock {

    typedef enum {
        START_CLOCK,
        STOP_CLOCK,
        HOLD_CLOCK,
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

        // Initialize the object using 'vcm'
        RetType init(VCM* vcm);

        // Initialize using the default VCM file
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

        // Block until a certain time
        // NOTE: blocking function
        RetType block_until(int64_t time);

        // Read the current time
        // NOTE: blocking function
        RetType read_time(int64_t* time);

        // Convert a int64 time to a string
        // outputed as T+/- hh:mm:ss:ms
        RetType to_str(int64_t time, std::string* str);

        // Update time of clock, intended to be updated by the 'clock' process
        // Contains the logic for hold time
        RetType tick();

    private:
        typedef struct {
            uint32_t readers;
            uint32_t writers;
            sem_t rmutex;
            sem_t wmutex;
            sem_t readTry;
            sem_t resource;
            int64_t time;
            int64_t hold_time;
        } clock_shm_t;

        static const int shmid = 0x92; // random unique int for shared memory key

        Shm* shm;
    };
}

#endif
