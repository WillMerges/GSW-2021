/********************************************************************
*  Name: basic.h
*
*  Purpose: Defines basic trigger functions
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include "lib/trigger/trigger.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/telemetry/TelemetryWriter.h"

using namespace trigger;

// takes two arguments
// first one is source, second one is destination
// copys source to destination
RetType COPY(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args);

// first arg is output measurement
// sums all other arguments and writes out to first arg
RetType SUM_UINT(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args);

// use Welford's method to take a rolling average of doubles
// averages the last 20 samples (nice for sampling 43kHz data at 10Hz)
// @arg1 newest sample (double)
// @arg2 mean (double)
RetType ROLLING_AVG_DOUBLE_20(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args);

// track maximum of a double value
// @arg1 newest sample (double)
// @arg2 maximum value (double)
RetType MAX_DOUBLE(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args);

// track minimum of a double value
// @arg1 newest sample (double)
// @arg2 minimum value (double)
RetType MIN_DOUBLE(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args);
