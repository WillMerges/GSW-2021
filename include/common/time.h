#ifndef GSW_TIME_H
#define GSW_TIME_H

#include <stdint.h>

// return the system time in milliseconds
// NOTE: does not account for timezone information (use gettimeofday instead)
static inline uint32_t systime() {
    uint32_t ms = 0;

    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);

    ms = time.tv_sec * 1000;
    ms += (time.tv_nsec / 1000000);

    return ms;
}

#endif
