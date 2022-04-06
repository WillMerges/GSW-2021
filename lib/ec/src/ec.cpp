/********************************************************************
*  Name: ec.cpp
*
*  Purpose: Engine Controller Library Functions
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include "lib/ec/ec.h"
#include <string.h>

// macros
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

// converts a solenoid state to OPEN or CLOSED
// @arg1 solenoid state (uint16)
// @arg2 output string (at least 5 char)
RetType SOLENOID_STATE_TO_STR(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    unsigned int state;

    if(unlikely(SUCCESS != tv->get_uint(args->args[0], &state))) {
        return FAILURE;
    }

    const char* str;
    size_t len;
    switch(state) {
        case 0:
            str = "CLOSED";
            len = 7;
            break;
        case 1:
            str = "OPEN";
            len = 5;
            break;
        default:
            str = "ERROR";
            len = 6;
            break;
    }

    return tw->write_raw(args->args[1], (uint8_t*)str, len);
}

// converts an igniter state to SPARK or OPEN
// @arg1 igniter state (uint16)
// @arg2 output string (at least 7 char)
RetType IGNITER_STATE_TO_STR(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    unsigned int state;

    if(unlikely(SUCCESS != tv->get_uint(args->args[0], &state))) {
        return FAILURE;
    }

    const char* str;
    size_t len;
    switch(state) {
        case 0:
            str = "OFF";
            len = 4;
            break;
        case 1:
            str = "SPARK";
            len = 6;
            break;
        default:
            str = "ERROR";
            len = 6;
            break;
    }

    return tw->write_raw(args->args[1], (uint8_t*)str, len);
}

// converts mode state to DISABLED, TEST, COLD, or HOT
// @arg1 mode state (uint16)
// @arg2 output string (at least 9 char)
RetType MODE_STATE_TO_STR(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    unsigned int state;

    if(unlikely(SUCCESS != tv->get_uint(args->args[0], &state))) {
        return FAILURE;
    }

    const char* str;
    size_t len;
    switch(state) {
        case 0:
            str = "DISABLED";
            len = 9;
            break;
        case 1:
            str = "COLD";
            len = 5;
            break;
        case 69:
            str = "TEST";
            len = 5;
            break;
        case 99:
            str = "HOT";
            len = 4;
            break;
        default:
            str = "ERROR";
            len = 6;
            break;
    }

    return tw->write_raw(args->args[1], (uint8_t*)str, len);
}

// converts safing state to IDLE or SAFING
// @arg1 mode state (uint16)
// @arg2 output string (at least 7 char)
RetType SAFE_STATE_TO_STR(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    unsigned int state;

    if(unlikely(SUCCESS != tv->get_uint(args->args[0], &state))) {
        return FAILURE;
    }

    const char* str;
    size_t len;
    switch(state) {
        case 0:
            str = "IDLE";
            len = 5;
            break;
        case 1:
            str = "SAFING";
            len = 7;
            break;
        default:
            str = "ERROR";
            len = 6;
            break;
    }

    return tw->write_raw(args->args[1], (uint8_t*)str, len);
}
