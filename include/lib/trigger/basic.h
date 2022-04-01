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
