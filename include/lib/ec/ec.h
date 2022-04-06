/********************************************************************
*  Name: ec.h
*
*  Purpose: Defines Engine Controller Library Functions
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#ifndef EC_H
#define EC_H

#include "common/types.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/telemetry/TelemetryWriter.h"
#include "lib/trigger/trigger.h"

#include <stdint.h>

typedef struct {
    uint32_t seq_num; // sequence number
    uint16_t control; // control
    uint16_t state;   // state
} ec_command_t;


// trigger functions
using namespace trigger;

// converts a solenoid state to OPEN or CLOSED
// @arg1 solenoid state (uint16)
// @arg2 output string (at least 6 char)
RetType SOLENOID_STATE_TO_STR(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args);

// converts an igniter state to SPARK or OFF
// @arg1 igniter state (uint16)
// @arg2 output string (at least 7 char)
RetType IGNITER_STATE_TO_STR(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args);

// converts mode state to DISABLED, TEST, COLD, or HOT
// @arg1 mode state (uint16)
// @arg2 output string (at least 9 char)
RetType MODE_STATE_TO_STR(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args);

// converts safing state to IDLE or SAFING
// @arg1 mode state (uint16)
// @arg2 output string (at least 7 char)
RetType SAFE_STATE_TO_STR(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args);

#endif
