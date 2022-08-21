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
#define TIMEOUT 200 // ms

// the amount of times to retransmit sending before erroring
#define NUM_RETRANSMITS 5

// value of this constant is the sequence number used
#define CONST_SEQUENCE "SEQUENCE"
// value of this constant is the network device commands are sent to
#define CONST_DEVICE "CONTROLLER_DEVICE"


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

    std::shared_ptr<VCM> veh;
    if(config_file == "") {
        veh = std::make_shared<VCM>(); // use default config file
    } else {
        veh = std::make_shared<VCM>(config_file); // use specified config file
    }

    // init VCM
    if(SUCCESS != veh->init()) {
        logger.log_message("failed to initialize VCM");
        return -1;
    }

    // parse constants file
    if(SUCCESS != veh->parse_consts()) {
        logger.log_message("failed to parse VCM constants");
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
    std::string CONST_DEVICE_STR = CONST_DEVICE;
    std::string* net_dev = veh->get_const(CONST_DEVICE_STR);
    if(nullptr == net_dev) {
        logger.log_message("missing constant: " + CONST_DEVICE_STR);
        return -1;
    }

    NetworkInterface net;
    if(SUCCESS != net.init(*net_dev, veh.get())) {
        logger.log_message("failed to open network interface");
        printf("failed to open network interface\n");

        return -1;
    }

    // initialize vlock lib
    if(SUCCESS != vlock::init()) {
        logger.log_message("failed to init vlock lib");
        printf("failed to init vlock lib\n");

        return -1;
    }

    // add sequence number measurement
    std::string CONST_SEQUENCE_STR = CONST_SEQUENCE;
    std::string* m = veh->get_const(CONST_SEQUENCE_STR);
    if(nullptr == m) {
        logger.log_message("missing constant: " + CONST_SEQUENCE_STR);
        return -1;
    }

    std::string meas = *m;
    if(FAILURE == tlm.add(meas)) {
        printf("failed to add measurement: %s\n", meas.c_str());
        logger.log_message("failed to add sequence number measurement");

        return -1;
    }

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
        printf("failed to get measurement: %s\n", meas.c_str());
        logger.log_message("failed to get sequence number measurement");

        // release the engine controller command resouce
        if(SUCCESS != vlock::unlock(vlock::ENGINE_CONTROLLER_COMMAND)) {
            printf("failed to unlock engine controller command resource\n");
            logger.log_message("failed to unlock engine controller command resource");
        }

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
    if(FAILURE == net.QueueUDPMessage((const uint8_t*)&cmd, sizeof(cmd))) {
        printf("failed to queue message to network interface\n");
        logger.log_message("failed to queue message to network interface");

        // release the engine controller command resouce
        if(SUCCESS != vlock::unlock(vlock::ENGINE_CONTROLLER_COMMAND)) {
            printf("failed to unlock engine controller command resource\n");
            logger.log_message("failed to unlock engine controller command resource");
        }

        return -1;
    }

    printf("command sent\n");

    // wait for our command to be acknowledged
    size_t retransmit_count = 0;
    uint32_t start = systime();
    uint32_t time_remaining = TIMEOUT;
    while(seq_num != cmd_num) {
        uint32_t now = systime();
        if(now >= start + TIMEOUT) {
            logger.log_message("timed out waiting for acknowledgement");
            printf("timed out waiting for acknowledgement\n");

            retransmit_count++;

            if(retransmit_count == NUM_RETRANSMITS) {
                // release the engine controller command resouce
                if(SUCCESS != vlock::unlock(vlock::ENGINE_CONTROLLER_COMMAND)) {
                    printf("failed to unlock engine controller command resource\n");
                    logger.log_message("failed to unlock engine controller command resource");
                }

                return -1;
            } else {
                logger.log_message("retransmitting command");
                printf("retransmitting command\n");

                start = now;
                continue;
            }
        } else {
            time_remaining = TIMEOUT - (now - start);
        }

        if(FAILURE == tlm.update(time_remaining)) {
            printf("failed to update telemetry\n");
            logger.log_message("failed to update telemetry");

            continue;
        } // otherwise success or timeout

        if(FAILURE == tlm.get_uint(meas, &seq_num)) {
            // this shouldn't happen
            printf("failed to get measurement: %s\n", meas.c_str());
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
