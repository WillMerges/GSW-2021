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
// @arg1 is raw input
// @arg2 is voltage output
RetType DAQ_ADC_SCALE(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args);

// calculates thermocouple values for a MAX31855K thermocouple board
// @arg1 raw input (32 bit int)
// @arg2 connected status
// @arg3 remote temperature in Celsius (double)
// @arg4 ambient temperature in Celsius (double)
RetType KTYPE_THERMOCOUPLE(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* arg);


#endif
