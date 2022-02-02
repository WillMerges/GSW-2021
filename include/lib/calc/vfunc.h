/********************************************************************
*  Name: vcalc.h
*
*  Purpose: User created functions to handle virtual telemetry
*           for virtual telemetry calculations.
*
*  NOTE:    This file should only be included ONCE by "calc.cpp"
*           If it is included by multiple files linked together
*           there will be a name collision.
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#ifndef VFUNC_H
#define VFUNC_H

#include "lib/calc/calc.h"
#include <string.h>

using namespace calc;
using namespace dls;
using namespace vcm;
using namespace convert;

// CONVERSION FUNCTIONS //

// directly copy a measurement
// 'out' must be the same size as 'in'
static RetType COPY(measurement_info_t* meas, uint8_t* dst, std::vector<arg_t>& args) {
    MsgLogger logger("CALC", "COPY");

    if(args.size() != 1) {
        logger.log_message("Incorrect number of args");
        return FAILURE;
    }

    measurement_info_t* in = args[0].meas;
    const uint8_t* src = args[0].addr;

    if(meas->size != in->size) {
        logger.log_message("Size mismatch");
        return FAILURE;
    }

    // need a special function for writing single measurement to shared memory!
    // ^^^ actually we're just writing to a passed in buffer, this is fine
    memcpy((void*)(dst), (void*)src, meas->size);

    return SUCCESS;
}

static RetType SUM_UINT(measurement_info_t* meas, uint8_t* dst, std::vector<arg_t>& args) {
    MsgLogger logger("CALC", "INTSUM");

    uint32_t sum = 0;
    uint32_t temp;

    // sum all arguments
    for(arg_t arg : args) {
        if(convert_to(veh, arg.meas, arg.addr, &temp) != SUCCESS) {
            logger.log_message("conversion failed");
            return FAILURE;
        }

        sum += temp;
    }

    // write out sum to output measurement
    if(convert_from(veh, meas, dst, sum) != SUCCESS) {
        logger.log_message("failed to convert sum to uint32");
        return FAILURE;
    }

    return SUCCESS;
}

// functions mapped to names //
typedef struct {
    const char* name;
    RetType (*func)(measurement_info_t*, uint8_t*, std::vector<arg_t>&);
} vfunc_t;

const vfunc_t vfunc_list[] =
{
    {"COPY", &COPY},
    {"SUM_UINT", &SUM_UINT},
    {NULL, NULL}
};

#endif
