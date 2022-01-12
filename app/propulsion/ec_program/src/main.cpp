/*
*  File:    main.cpp
*
*  Purpose: Engine Controller program. Reads an Engine Controller program and
*           executes all commands. Programs are either interpretted from standard
*           input or read from a file.
*
*  Usage:   ./ec_program <-f VCM config file>
*/
#include "lib/ec/ec.h"
#include "lib/nm/nm.h"
#include "lib/vcm/vcm.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/dls/dls.h"
#include "common/types.h"
#include "common/time.h"
#include "lib/clock/clock.h"
#include "lib/vlock/vlock.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

using namespace dls;
using namespace vcm;
using namespace nm;
using namespace countdown_clock;

// amount of time to wait for telemetry to indicate command was accepted
#define TIMEOUT 500 // ms

// measurement to look for sequence number of command in
#define SEQUENCE_ACK_MEASUREMENT "SEQ_NUM"

// engine controller command
typedef struct command_s{
    int64_t time;
    uint16_t control;
    uint16_t state;

    // for sorting my time
    bool operator < (const struct command_s& other) const
    {
        return time < other.time;
    }
} command_t;

// maps macros
std::unordered_map<std::string, std::string> macros;

// keys are time, values are commands
std::vector<command_t> commands;

// Countdown Clock
CountdownClock cl;


void release_resources() {
    MsgLogger logger("EC_PROGRAM", "release_resources");

    // unlock the engine controller resource
    if(SUCCESS != vlock::unlock(vlock::ENGINE_CONTROLLER_PROGRAM)) {
        printf("failed to unlock engine controller resource\n");
        logger.log_message("failed to unlock engine controller resource");
    }
}

static bool ignore_sig = false;

// signal handler
void sighandler(int signum) {
    MsgLogger logger("SHMCTRL", "sighandler");

    if(ignore_sig) {
        return;
    }

    release_resources();

    exit(signum);
}

// parse the program file
RetType parse() {
    MsgLogger logger("EC_PROGRAM", "parse");

    uint32_t curr_time = 0; // ms

    for(std::string line; std::getline(std::cin, line); ) {
        if(line == "" || !line.rfind("#",0)) { // blank or comment '#'
            continue;
        }

        std::stringstream ss(line);
        std::string s;
        std::vector<std::string> tokens;

        while(getline(ss, s, ' ')) {
            if(!s.rfind("#",0)) {
                // comment
                break;
            }

            if(s == "" || s == " ") {
                continue;
            }

            tokens.push_back(s);
        }

        if(tokens.size() == 1) {
            if(tokens[0] == "end") {
                break;
            }
        }

        if(tokens.size() != 3) {
            printf("unexpected number of tokens\n");

            for(std::string token : tokens) {
                std::cout << token << '\n';
            }

            logger.log_message("unexpected number of tokens");
            return FAILURE;
        }

        if(tokens[1] == "=") {
            // macro
            macros[tokens[0]] = tokens[2];
        } else {
            // replace with macros if available
            for(size_t i = 0; i < tokens.size(); i++) {
                if(macros.find(tokens[i]) != macros.end()) {
                    // we have a macro for this, replace the token
                    tokens[i] = macros[tokens[i]];
                }
            }

            long abs_time = strtol(tokens[0].c_str(), NULL, 10);

            if(abs_time == 0 && errno == EINVAL) {
                printf("failed to convert time value\n");
                logger.log_message("time value invalid");

                return FAILURE;
            }

            if(abs_time < curr_time) {
                printf("absolute time is less than current time, invalid command!\n");
                logger.log_message("absolute time is less than current time, invalid command!");

                return FAILURE;
            } else {
                curr_time = abs_time;
            }

            long control = strtol(tokens[1].c_str(), NULL, 10);

            if(control == 0 && errno == EINVAL) {
                printf("failed to convert control value\n");
                logger.log_message("control value invalid");

                return FAILURE;
            }

            long state = strtol(tokens[2].c_str(), NULL, 10);

            if(state == 0 && errno == EINVAL) {
                printf("failed to convert state value\n");
                logger.log_message("state value invalid");

                return FAILURE;
            }

            // assemble the command and add it
            command_t cmd;
            cmd.control = (uint16_t)control;
            cmd.state = (uint16_t)state;
            cmd.time = (int64_t)abs_time;
            commands.push_back(cmd);
        }
    }

    // sort the commands by execution time
    std::sort(commands.begin(), commands.end());

    return SUCCESS;
}

// main function
int main(int argc, char* argv[]) {
    MsgLogger logger("EC_PROGRAM", "main");

    logger.log_message("executing engine controller program");

    // interpret the next argument as a config_file location if available
    std::string config_file = "";
    if(argc > 2) {
        if(!strcmp(argv[1], "-f")) {
            config_file = argv[2];
        } else {
            printf("if specifying config file, must use -f flag\n");
            printf("usage: ./ec_cmd [program] <-f VCM config file>\n");
            logger.log_message("invalid config file argument");

            return -1;
        }
    }

    // initialize the vlock library
    if(SUCCESS != vlock::init()) {
        printf("failed to init vlock library\n");
        logger.log_message("failed to init vlock library");

        return -1;
    }

    // add signal handlers
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGFPE, sighandler);
    signal(SIGABRT, sighandler);

    // we need to ignore any signals until we know we have the lock
    // if we get a signal before trying the lock we may decrement the lock when we didn't own it
    ignore_sig = true;

    // lock the engine controller program resource
    if(SUCCESS != vlock::try_lock(vlock::ENGINE_CONTROLLER_PROGRAM)) {
        printf("failed to lock engine controller resource\n");
        logger.log_message("failed to lock engine controller resource");

        return -1;
    }

    ignore_sig = false;

    // try and parse program
    if(SUCCESS != parse()) {
        printf("failed to parse program\n");
        logger.log_message("failed to parse program\n");

        release_resources();
        return -1;
    }

    // Open the countdown clock
    if(FAILURE == cl.init()) {
        logger.log_message("failed to initialize countdown clock");
        printf("failed to initialize countdown clock\n");

        release_resources();
        return -1;
    }

    if(FAILURE == cl.open()) {
        logger.log_message("failed to open countdown clock");
        printf("failed to open countdown clock\n");

        release_resources();
        return -1;
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
        release_resources();
        return -1;
    }

    // initialize telemetry viewer
    TelemetryViewer tlm;
    if(FAILURE == tlm.init(veh)) {
        logger.log_message("failed to initialize telemetry viewer");
        printf("failed to initialize telemetry viewer\n");

        release_resources();
        return -1;
    }

    // initialize network interface
    NetworkInterface net((veh->device).c_str());
    if(FAILURE == net.Open()) {
        logger.log_message("failed to open network interface");
        printf("failed to open network interface\n");

        release_resources();
        return -1;
    }

    // add sequence number measurement
    std::string meas = SEQUENCE_ACK_MEASUREMENT;
    if(FAILURE == tlm.add(meas)) {
        printf("failed to add measurement: %s\n", SEQUENCE_ACK_MEASUREMENT);
        logger.log_message("failed to add sequence number measurement");

        release_resources();
        return -1;
    }
    tlm.set_update_mode(TelemetryViewer::BLOCKING_UPDATE);

    // shouldn't block on first update
    if(SUCCESS != tlm.update(TIMEOUT)) {
        printf("have not received any telemetry, cannot execute program\n");
        logger.log_message("no telemetry received, cannot execute program");

        release_resources();
        return -1;
    }

    // wait until the clock is started
    int64_t curr_time;
    bool stopped = true;
    while(stopped) {
        if(FAILURE == cl.read_time(&curr_time, &stopped)) {
            logger.log_message("failed to read countdown clock");
            printf("failed to read countdown clock");

            release_resources();
            return -1;
        }

        printf("still stopped\n");
        fflush(stdout);
        usleep(1000); // sleep 1 ms
    }

    // get the current sequence number
    // TODO there could be an issue of sequence numbers if multiple commands execute at once
    //      e.g. the one we read is out of date (have a way to lock physical resources?)
    unsigned int seq_num;
    if(SUCCESS != tlm.get_uint(meas, &seq_num)) {
        // this shouldn't happen
        printf("failed to get measurement: %s\n", SEQUENCE_ACK_MEASUREMENT);
        logger.log_message("failed to get sequence number measurement");

        release_resources();
        return -1;
    }

    // execute all parsed commands
    ec_command_t cmd;
    long cmd_num;
    command_t c;
    for(int i = 0; i < (int)commands.size(); i++) {
        c = commands[i];

        if(FAILURE == cl.read_time(&curr_time, &stopped)) {
            logger.log_message("failed to read countdown clock");
            printf("failed to read countdown clock");

            return -1;
        }

        if(stopped) {
            logger.log_message("clock was stopped, exiting program");
            printf("clock was stopped, exiting program\n");

            return -1;
        }

        if(curr_time < c.time) {
            // we need to wait to execute this
            usleep((c.time - curr_time) * 1000);
            i--;
            continue;
        }

        // generate command
        cmd_num = seq_num + 1;
        cmd.seq_num = (uint32_t)cmd_num;
        cmd.control = c.control;
        cmd.state = c.state;

        // until our command has been ACK'd we don't want to exit on a signal
        // this way we can be sure to unlock things
        ignore_sig = true;

        // lock the engine controller so no other commands are sent
        if(SUCCESS != vlock::lock(vlock::ENGINE_CONTROLLER_COMMAND, 1)) {
            // either error or it took longer than 1ms to obtain the lock
            logger.log_message("failed to obtain engine controller command resource");
            printf("ailed to obtain engine controller command resource\n");

            release_resources();
            return -1;
        }

        // send the command over the network
        if(FAILURE == net.QueueUDPMessage((char*)&cmd, sizeof(cmd))) {
            printf("failed to queue message to network interface\n");
            logger.log_message("failed to queue message to network interface");

            return -1;
        }

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

        // unlock the command resource
        if(SUCCESS != vlock::unlock(vlock::ENGINE_CONTROLLER_COMMAND)) {
            logger.log_message("failed to unlock engine controller command resource");
            printf("failed to unlock engine controller command resource\n");

            release_resources();

            return -1;
        }

        // re-enable exiting on signal
        ignore_sig = false;
    }

    // unlock the engine controller program resource
    if(SUCCESS != vlock::unlock(vlock::ENGINE_CONTROLLER_PROGRAM)) {
        printf("failed to unlock engine controller resource\n");
        logger.log_message("failed to unlock engine controller program resource");

        return -1;
    }
}
