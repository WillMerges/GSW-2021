/*
*  File:    main.cpp
*
*  Purpose: Engine Controller program. Reads an Engine Controller program and
*           executes all commands. Programs in '.ec' format, samples in 'programs'
*
*  Usage:   ./ec_program [program] <VCM config file>
*/
#include "lib/ec/ec.h"
#include "lib/nm/nm.h"
#include "lib/vcm/vcm.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/dls/dls.h"
#include "common/types.h"
#include <unordered_map>
#include <map>
#include <string>
#include <stdint.h>

using namespace dls;
using namespace vcm;
using namespace nm;

// amount of time to wait for telemetry to indicate command was accepted
#define TIMEOUT 500 // ms

// measurement to look for sequence number of command in
#define SEQUENCE_ACK_MEASUREMENT "SEQ_NUM"


// maps macros
std::unordered_map<std::string, std::string> macros;

// engine controller command
typedef struct {
    uint16_t control;
    uint16_t state;
} command_t;

// keys are time, values are commands
std::map commands<uint32_t, command_t>;


// parse the program file
bool parse_file(const char* file) {
 // TODO
}

// main function
int main(int argc, char* argv[]) {
    MsgLogger logger("EC_PROGRAM", "main");

    if(argc < 2) {
        printf("usage: ./ec_cmd [program] <VCM config file>\n");
        logger.log_message("invalid arguments");

        return -1;
    }

    // try and parse program file
    if(!parse_file(argv[1])) {
        printf("failed to parse program\n");
        logger.log_message("failed to parse program\n");

        return -1;
    }

    // interpret the 2nd argument as a config_file location if available
    std::string config_file = "";
    if(argc > 2) {
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
    tlm.set_update_mode(TelemetryViewer::BLOCKING_UPDATE);

    // shouldn't block on first update
    if(SUCCESS != tlm.update(TIMEOUT)) {
        printf("have not received any telemetry, cannot execute program\n");
        logger.log_message("no telemetry received, cannot execute program");

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

    // TODO execute all parsed commands
}
