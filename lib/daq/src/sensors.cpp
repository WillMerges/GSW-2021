/********************************************************************
*  Name: sensors.cpp
*
*  Purpose: Functions relating to data acquisition sensors
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include "lib/daq/sensors.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/telemetry/TelemetryWriter.h"
#include "lib/trigger/trigger.h"
#include "common/types.h"

using namespace trigger;

// macros
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)


RetType DAQ_ADC_SCALE(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    // NOTE: we don't do an arg check!
    // if it segfaults, that's the fault of whoever made the triggers file wrong

    int32_t data;

    if(unlikely(SUCCESS != tv->get_int(args->args[0], &data))) {
        return FAILURE;
    }

    static const double vref = 2.442; // volts

    double result = (double)data * vref / (double)(1 << 23);

    if(unlikely(tw->write(args->args[1], (uint8_t*)&result, sizeof(double)))) {
        return FAILURE;
    }

    return SUCCESS;
}
