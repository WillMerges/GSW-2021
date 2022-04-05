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

// *** includes for trigger functions *** //
#include "lib/trigger/basic.h"
#include "lib/daq/sensors.h"
// ************************************** //


typedef struct {
    const char* name;
    trigger_handle func;
} trigger_map_t;

// TODO add min and max # of args

// *** modify this mapping to add triggers *** //
const trigger_map_t trigger_func_list[] =
{
    {"COPY", &COPY},
    {"SUM_UINT", &SUM_UINT},
    {"ROLLING_AVG_DOUBLE_430", &ROLLING_AVG_DOUBLE_430},
    {"MAX_DOUBLE", &MAX_DOUBLE},
    {"MIN_DOUBLE", &MIN_DOUBLE},
    {"DAQ_ADC_SCALE", &DAQ_ADC_SCALE},
    {"MAX31855K_THERMOCOUPLE", &MAX31855K_THERMOCOUPLE},
    {"PCB1403_CURRENT_EXCITE", &PCB1403_CURRENT_EXCITE},
    {"PRESSURE_TRANSDUCER_8252", &PRESSURE_TRANSDUCER_8252},
    {NULL, NULL}
};
// ******************************************* //

#endif
