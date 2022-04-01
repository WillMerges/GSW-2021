/********************************************************************
*  Name: trigger.h
*
*  Purpose: Functions that handle measurement triggers.
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#ifndef TRIGGER_H
#define TRIGGER_H

#include "common/types.h"
#include "lib/vcm/vcm.h"
#include "lib/telemetry/TelemetryWriter.h"
#include "lib/telemetry/TelemetryReader.h"
#include <stdlib.h>
#include <vector>

using namespace vcm;

namespace trigger {

    typedef struct {
        measurement_info_t** args;
        size_t num_args;
    } arg_t;

    typedef void (*trigger_handle)(TelemetryViewer*, TelemetryWriter*, arg_t*);

    typedef struct {
        size_t unique_id;
        trigger_handle func;
        arg_t* args;
    } trigger_t;

    // parse the trigger file listed in the vcm config file into a vector of triggers
    // returns FILENOTFOUND if there is no trigger file exists
    // returns FAILURE if the file is invalid or on other error
    // return SUCCESS on a successful parse
    RetType parse_trigger_file(VCM* veh, std::vector<trigger_t*>* triggers);
}


#endif
