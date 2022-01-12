#include "lib/ec/ec.h"
#include "lib/nm/nm.h"
#include "lib/vcm/vcm.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/dls/dls.h"
#include "lib/vlock/vlock.h"
#include "common/types.h"
#include "common/time.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <signal.h>

// send a command to the engine controller
// usage: ./ec_cmd [control] [state] <VCM config file>
//   config file is optional

using namespace dls;
using namespace vcm;
using namespace nm;

// amount of time to wait for telemetry to indicate command was accepted
#define TIMEOUT 500 // ms

// measurement to look for sequence number of command in
#define SEQUENCE_ACK_MEASUREMENT "SEQ_NUM"


void sighandler(int) {
    // don't exit, ignore the signal
    return;
}

int main(int argc, char* argv[]) {
    MsgLogger logger("EC_CMD", "main");

    if(argc < 3) {
        printf("usage: ./ec_cmd [control] [state] <VCM config file>\n");
        logger.log_message("invalid arguments");

        return -1;
    }

    long control = 0;
    control = strtol(argv[1], NULL, 10);

    if(control == 0 && errno == EINVAL) {
        printf("failed to convert control argument\n");
        logger.log_message("control argument invalid");

        return -1;
    }

    long state = 0;
    state = strtol(argv[2], NULL, 10);

    if(state == 0 && errno == EINVAL) {
        printf("failed to convert state argument\n");
        logger.log_message("state argument invalid");

        return -1;
    }

    // interpret the 3rd argument as a config_file location if available
    std::string config_file = "";
    if(argc > 3) {
        config_file = argv[3];
    }

    VCM* veh;
    if(config_file == "") {
        veh = new VCM(); // use default config file
    } else {
        veh = new VCM(config_file); // use specified config file
    }

    // init VCM
    if(veh->init() == FAILURE) {
        logger.log_message("failed to initialize VCM");
        return -1;
    }

    // initialize telemetry viewer
    TelemetryViewer tlm;
    if(FAILURE == tlm.init(veh)) {
        logger.log_message("failed to initialize telemetry viewer");
        printf("failed to initialize telemetry viewer\n");

        return -1;
    }

    // initialize network interface
    NetworkInterface net((veh->device).c_str());
    if(FAILURE == net.Open()) {
        logger.log_message("failed to open network interface");
        printf("failed to open network interface\n");

        return -1;
    }

    // add sequence number measurement
    std::string meas = SEQUENCE_ACK_MEASUREMENT;
    if(FAILURE == tlm.add(meas)) {
        printf("failed to add measurement: %s\n", SEQUENCE_ACK_MEASUREMENT);
        logger.log_message("failed to add sequence number measurement");

        return -1;
    }
    // tlm.add_all();
    tlm.set_update_mode(TelemetryViewer::BLOCKING_UPDATE);

    // shouldn't block on first update
    if(SUCCESS != tlm.update(TIMEOUT)) {
        printf("have not received any telemetry, cannot send command\n");
        logger.log_message("no telemetry received, cannot send command");

        return -1;
    }

    // add signal handlers to ignore signals
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGFPE, sighandler);
    signal(SIGABRT, sighandler);

    // obtain the engine controller command resouce
    if(SUCCESS != vlock::try_lock(vlock::ENGINE_CONTROLLER_COMMAND)) {
        printf("failed to obtain engine controller command resource\n");
        logger.log_message("failed to obtain engine controller command resource");

        return -1;
    }

    // get the current sequence number
    unsigned int seq_num;
    if(SUCCESS != tlm.get_uint(meas, &seq_num)) {
        // this shouldn't happen
        printf("failed to get measurement: %s\n", SEQUENCE_ACK_MEASUREMENT);
        logger.log_message("failed to get sequence number measurement");

        return -1;
    }

    // use a higher sequence number for this command
    unsigned int cmd_num = seq_num + 1;

    // create the command to send
    ec_command_t cmd;
    cmd.seq_num = (uint32_t)cmd_num;
    cmd.control = (uint16_t)control;
    cmd.state = (uint16_t)state;

    // send the command over the network
    if(FAILURE == net.QueueUDPMessage((char*)&cmd, sizeof(cmd))) {
        printf("failed to queue message to network interface\n");
        logger.log_message("failed to queue message to network interface");

        return -1;
    }

    printf("command sent\n");

    // wait for our command to be acknowledged
    uint32_t start = systime();
    uint32_t time_remaining = TIMEOUT;
    while(seq_num != cmd_num) {
        uint32_t now = systime();
        if(now >= start + TIMEOUT) {
            logger.log_message("timed out waiting for acknowledgement");
            printf("timed out waiting for acknowledgement\n");

            return -1;
        } else {
            time_remaining = TIMEOUT - (now - start);
        }

        if(FAILURE == tlm.update(time_remaining)) {
            printf("failed to update telemetry\n");
            logger.log_message("failde to update telemetry");

            continue;
        } // otherwise success or timeout

        if(FAILURE == tlm.get_uint(meas, &seq_num)) {
            // this shouldn't happen
            printf("failed to get measurement: %s\n", SEQUENCE_ACK_MEASUREMENT);
            logger.log_message("failed to get sequence number measurement");

            continue;
        } // okay if we get timeout, catch it next time around
    }

    printf("command acknowledged\n");


    // release the engine controller command resouce
    if(SUCCESS != vlock::unlock(vlock::ENGINE_CONTROLLER_COMMAND)) {
        printf("failed to unlock engine controller command resource\n");
        logger.log_message("failed to unlock engine controller command resource");

        return -1;
    }

    return 0;
}
