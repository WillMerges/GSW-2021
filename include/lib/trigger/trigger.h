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
#include "lib/telemetry/TelemetryViewer.h"
#include <stdlib.h>
#include <vector>

using namespace vcm;

namespace trigger {

    typedef struct {
        measurement_info_t** args;
        size_t num_args;
    } arg_t;

    // takes a telemetry reader and writer and a list of args
    // returns SUCCESS if new value was written, anything else otherwise
    typedef RetType (*trigger_handle)(TelemetryViewer*, TelemetryWriter*, arg_t*);

    typedef struct {
        measurement_info_t* meas; // measurement that causes the trigger
        size_t unique_id;         // some unique id
        trigger_handle func;      // function to call when measurement is updated
        arg_t args;               // arguments to call function with
    } trigger_t;

    // parse the trigger file listed in the vcm config file into a vector of triggers
    // returns FILENOTFOUND if there is no trigger file exists
    // returns FAILURE if the file is invalid or on other error
    // return SUCCESS on a successful parse
    RetType parse_trigger_file(VCM* veh, std::vector<trigger_t>* triggers);
}


#endif
