/********************************************************************
*  Name: basic.cpp
*
*  Purpose: Basic trigger functions
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include <string.h>
#include "lib/trigger/basic.h"
#include "lib/trigger/trigger.h"
#include "lib/convert/convert.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/telemetry/TelemetryWriter.h"
#include "lib/dls/dls.h"

using namespace dls;
using namespace trigger;

// macros
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

RetType COPY(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    MsgLogger logger("trigger_basic", "COPY");

    if(args->args[0]->size != args->args[1]->size) {
        logger.log_message("size mismatch");
        return FAILURE;
    }

    uint8_t* src;

    if(SUCCESS != tv->get_raw(args->args[0], &src)) {
        logger.log_message("read fail, arg 0");
    }

    return tw->write_raw(args->args[1], src, args->args[0]->size);
}

RetType SUM_UINT(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    MsgLogger logger("trigger_basic", "SUM_UINT");

    uint32_t temp;
    uint32_t sum = 0;

    for(size_t i = 1; i < args->num_args; i++) {
        if(SUCCESS != tv->get_uint(args->args[i], &temp)) {
            logger.log_message("uint conversion failure");
            return FAILURE;
        }

        sum += temp;
    }

    return tw->write(args->args[0], (uint8_t*)&sum, sizeof(sum));
}

// @ar1 newest sample (double)
// @arg2 last mean (double)
RetType ROLLING_AVG_DOUBLE_20(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    double m;
    if(unlikely(SUCCESS != tv->get_double(args->args[1], &m))) {
        return FAILURE;
    }

    double x;
    if(unlikely(SUCCESS != tv->get_double(args->args[0], &x))) {
        return FAILURE;
    }

    // Welford's method
    m = m + ((x - m) / 20);

    if(unlikely(SUCCESS != tw->write(args->args[1], (uint8_t*)&m, sizeof(double)))) {
        return FAILURE;
    }

    return SUCCESS;
}

// track maximum of a double value
// @arg1 newest sample (double)
// @arg2 maximum value (double)
RetType MAX_DOUBLE(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    double x;
    if(unlikely(SUCCESS != tv->get_double(args->args[0], &x))) {
        return FAILURE;
    }

    double max;
    if(unlikely(SUCCESS != tv->get_double(args->args[1], &max))) {
        return FAILURE;
    }

    if(x > max) {
        if(unlikely(SUCCESS != tw->write(args->args[1], (uint8_t*)&x, sizeof(double)))) {
            return FAILURE;
        }

        return SUCCESS;
    }

    return NOCHANGE;
}

// track minimum of a double value
// @arg1 newest sample (double)
// @arg2 minimum value (double)
RetType MIN_DOUBLE(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    double x;
    if(unlikely(SUCCESS != tv->get_double(args->args[0], &x))) {
        return FAILURE;
    }

    double max;
    if(unlikely(SUCCESS != tv->get_double(args->args[1], &max))) {
        return FAILURE;
    }

    if(x < max) {
        if(unlikely(SUCCESS != tw->write(args->args[1], (uint8_t*)&x, sizeof(double)))) {
            return FAILURE;
        }

        return SUCCESS;
    }

    return NOCHANGE;
}

// derivate velocity from multiple position measurements
// @arg1 newest sample (double)
// @arg2 output velocity (double)
static double last_p = 0.0;
static uint64_t last_us = 0;
RetType VELOCITY_DOUBLE(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    double p;
    if(unlikely(SUCCESS != tv->get_double(args->args[0], &p))) {
        return FAILURE;
    }

    double delta_p = last_p - p;
    last_p = p;

    if(unlikely(last_us == 0)) {
        double v = 0.0;

        if(unlikely(SUCCESS != tw->write(args->args[1], (uint8_t*)&v, sizeof(double)))) {
            return FAILURE;
        }
    } else {
        struct timespec time;
        if(0 != clock_gettime(CLOCK_MONOTONIC_RAW, &time)) {
            return FAILURE;
        }

        uint64_t curr_time = (time.tv_sec * 1000000) + (time.tv_nsec / 1000);
        uint64_t delta_us = curr_time - last_us;
        last_us = curr_time;

        double delta_t = delta_us / 1000000;

        double v = delta_p / delta_t;

        if(unlikely(SUCCESS != tw->write(args->args[1], (uint8_t*)&v, sizeof(double)))) {
            return FAILURE;
        }
    }

    return SUCCESS;
}
