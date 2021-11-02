/*******************************************************************************
* Name: reaper.cpp
*
* Purpose: Wakes up anyone blocked on a packet in shared memory.
*          Intended purpose is to be executed from a killed processes signal
*          handler. The signal handler sets a killed flag but it may be blocked.
*          Calling the 'futex' syscall from within a signal handler does not
*          work and a signal does not interrupt the 'futex' syscall, so we need
*          a special 'reaper' process to wake it up so it can die.
*
* Usage: ./reaper [vcm config file] [bit mask]
*        'bit mask' represents which packet id's need to be awoken
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include "common/types.h"
#include "lib/vcm/vcm.h"
#include "lib/dls/dls.h"
#include "lib/telemetry/TelemetryViewer.h"

// don't fear the reaper

using namespace vcm;
using namespace dls;


int main(int argc, char** argv) {
    MsgLogger logger("REAPER", "main");

    if(argc < 3) {
        printf("usage: ./reaper [vcm config file] [packet_id]\n");
        logger.log_message("invalid arguments");
        exit(-1);
    }

    std::string msg = "waking processes: ";
    msg += argv[1];
    msg += " ";
    msg += argv[2];
    logger.log_message(msg);

    VCM veh(argv[1]);

    if(SUCCESS != veh.init()) {
        printf("failed to initialize VCM\n");
        logger.log_message("failed to initialize VCM");
        exit(-1);
    }

    uint32_t mask;

    if(1 != sscanf(argv[2], "%u", &mask)) {
        printf("failed to match argument 2 to mask\n");
        logger.log_message("failed to match argument 2 to mask");
        exit(-1);
    }

    TelemetryShm tlm;

    if(SUCCESS != tlm.init(&veh)) {
        printf("failed to initialize telemetry shared memory\n");
        logger.log_message("failed to initialize telemetry shared memory");
        exit(-1);
    }

    if(SUCCESS != tlm.open()) {
        printf("failed to attach to telemetry shared memory\n");
        logger.log_message("failed to attach to telemetry shared memory");
        exit(-1);
    }

    if(SUCCESS != tlm.force_wake(mask)) {
        printf("force wake operation failed\n");
        logger.log_message("force wake operation failed");
        exit(-1);
    }

    logger.log_message("success");
    return 0;
}
