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

// *** modify this mapping to add triggers *** //
const trigger_map_t trigger_func_list[] =
{
    {"COPY", &COPY},
    {"SUM_UINT", &SUM_UINT},
    {"DAQ_ADC_SCALE", &DAQ_ADC_SCALE},
    {"KTYPE_THERMOCOUPLE", &KTYPE_THERMOCOUPLE},
    {NULL, NULL}
};
// ******************************************* //

#endif
