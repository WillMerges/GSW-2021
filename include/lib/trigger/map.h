/********************************************************************
*  Name: map.h
*
*  Purpose: Maps trigger names to actual functions
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#ifndef TRIGGER_MAP_H
#define TRIGGER_MAP_H

#include "lib/trigger/trigger.h"
#include <limits.h>

// *** includes for trigger functions *** //
#include "lib/trigger/basic.h"
#include "lib/daq/sensors.h"
#include "lib/ec/ec.h"
// ************************************** //


typedef struct {
    const char* name;       // name that appears in trigger config file
    trigger_handle func;    // function to call for this trigger
    size_t min_args;        // minimum number of arguments
    size_t max_args;        // maximum number of arguments
} trigger_map_t;

// *** modify this mapping to add triggers *** //
const trigger_map_t trigger_func_list[] =
{
    {"COPY", &COPY, 2, 2},
    {"SUM_UINT", &SUM_UINT, 3, UINT_MAX},
    {"ROLLING_AVG_DOUBLE_20", &ROLLING_AVG_DOUBLE_20, 2, 2},
    {"MAX_DOUBLE", &MAX_DOUBLE, 2, 2},
    {"MIN_DOUBLE", &MIN_DOUBLE, 2, 2},
    {"DAQ_ADC_SCALE", &DAQ_ADC_SCALE, 2, 2},
    {"MAX31855K_THERMOCOUPLE", &MAX31855K_THERMOCOUPLE, 5, 5},
    {"PCB1403_CURRENT_EXCITE", &PCB1403_CURRENT_EXCITE, 2, 2},
    {"PRESSURE_TRANSDUCER_8252", &PRESSURE_TRANSDUCER_8252, 2, 2},
    {"SOLENOID_STATE_TO_STR", &SOLENOID_STATE_TO_STR, 2, 2},
    {"IGNITER_STATE_TO_STR", &IGNITER_STATE_TO_STR, 2, 2},
    {"MODE_STATE_TO_STR", &MODE_STATE_TO_STR, 2, 2},
    {"SAFE_STATE_TO_STR", &SAFE_STATE_TO_STR, 2, 2},
    {NULL, NULL, 0, 0}
};
// ******************************************* //

#endif
