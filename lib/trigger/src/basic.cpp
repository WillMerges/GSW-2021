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

    if(args->num_args != 2) {
        logger.log_message("invalid number of arguments");
        return FAILURE;
    }

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

    if(args->num_args < 1) {
        logger.log_message("requires at least one argument");
        return FAILURE;
    }

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
RetType ROLLING_AVG_DOUBLE_430(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    // NOTE: no arg checking (has to be fast)

    double m;
    if(unlikely(SUCCESS != tv->get_double(args->args[1], &m))) {
        return FAILURE;
    }

    double x;
    if(unlikely(SUCCESS != tv->get_double(args->args[0], &x))) {
        return FAILURE;
    }

    // Welford's method
    m = m + ((x - m) / 430);

    if(unlikely(SUCCESS != tw->write(args->args[1], (uint8_t*)&m, sizeof(double)))) {
        return FAILURE;
    }

    return SUCCESS;
}

// track maximum of a double value
// @arg1 newest sample (double)
// @arg2 maximum value (double)
RetType MAX_DOUBLE(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    // NOTE: no arg checking, fast!

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
    // NOTE: no arg checking, fast!

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
