/********************************************************************
*  Name: sensors.h
*
*  Purpose: Functions relating to data acquisition sensors
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#ifndef DAQ_SENSORS_H
#define DAQ_SENSORS_H

#include "common/types.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/telemetry/TelemetryWriter.h"
#include "lib/trigger/trigger.h"

using namespace trigger;

// scales an ADC voltage to actual voltage
// argument one is raw input
// argument two is voltage output
RetType DAQ_ADC_SCALE(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args);

#endif
