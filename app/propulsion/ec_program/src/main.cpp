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
#include <limits.h>

using namespace dls;
using namespace vcm;
using namespace nm;
using namespace countdown_clock;

// amount of time to wait for telemetry to indicate command was accepted
#define TIMEOUT 20 // ms

// the amount of times to retransmit sending before erroring
#define NUM_RETRANSMITS 5

// value of this constant is the sequence number used
#define CONST_SEQUENCE "SEQUENCE"
// value of this constant is the network device commands are sent to
#define CONST_DEVICE "CONTROLLER_DEVICE"

// engine controller command
struct command_s {
    uint16_t control;
    uint16_t state;
};

// action
typedef union {
    struct command_s cmd;
    std::string* msg;
} action_t;

typedef enum {
    COMMAND,
    MESSAGE
} action_type_t;

typedef struct event_s {
    int64_t time;
    action_type_t type;
    action_t action;

    // for sorting by time
    bool operator < (struct event_s& other) const {
        return time < other.time;
    }
} event_t;

// maps macros
std::unordered_map<std::string, std::string> macros;

// keys are time, values are commands
std::vector<event_t> events;

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

    long curr_time = LONG_MIN; // ms

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

        if(tokens.size() == 0) {
            continue;
        }

        if(tokens.size() == 1) {
            if(tokens[0] == "end") {
                break;
            }
        }

        if(tokens[1] == "MSG") {
            // logging message
            long abs_time = strtol(tokens[0].c_str(), NULL, 10);

            if(abs_time == 0 && errno == EINVAL) {
                printf("failed to convert time value\n");
                logger.log_message("time value invalid");

                return FAILURE;
            }

            if(abs_time < curr_time) {
                printf("absolute time (%li) is less than current time (%li), invalid command!\n", abs_time, curr_time);
                logger.log_message("absolute time is less than current time, invalid command!");

                return FAILURE;
            } else {
                curr_time = abs_time;
            }

            event_t ev;
            ev.time = (int64_t)abs_time;
            ev.type = MESSAGE;
            ev.action.msg = new std::string();
            *(ev.action.msg) = "";

            if(tokens[2][0] != '\"') {
                printf("message commands must list a string surrounded by \"'s\n");
                logger.log_message("message commands must list a string surrounded by \"'s");

                return FAILURE;
            }

            tokens[2].erase(0, 1);

            size_t i = 2;
            for(; i < tokens.size() - 1; i++) {
                *(ev.action.msg) += tokens[i] + " ";
            }

            if(tokens[i].back() != '\"') {
                printf("message commands must list a string surrounded by \"'s\n");
                logger.log_message("message commands must list a string surrounded by \"'s");

                return FAILURE;
            }

            tokens[i].pop_back();
            *(ev.action.msg) += tokens[i];

            events.push_back(ev);
        }
        else if(tokens.size() == 3) {
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
                    printf("absolute time (%li) is less than current time (%li), invalid command!\n", abs_time, curr_time);
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
                event_t ev;
                ev.time = (int64_t)abs_time;
                ev.type = COMMAND;
                ev.action.cmd.control = (uint16_t)control;
                ev.action.cmd.state = (uint16_t)state;
                events.push_back(ev);
            }
        } else {
            printf("unexpected number of tokens\n");

            for(std::string token : tokens) {
                std::cout << token << '\n';
            }

            logger.log_message("unexpected number of tokens");
            return FAILURE;
        }
    }

    // sort the commands by execution time
    std::sort(events.begin(), events.end());

    return SUCCESS;
}

// main function
int main(int argc, char* argv[]) {
    MsgLogger logger("EC_PROGRAM", "main");

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
    if(SUCCESS != veh->init()) {
        logger.log_message("failed to initialize VCM");
        release_resources();
        return -1;
    }

    if(SUCCESS != veh->parse_consts()) {
        logger.log_message("failed to parse VCM constants");
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
    std::string CONST_DEVICE_STR = CONST_DEVICE;
    std::string* net_dev = veh->get_const(CONST_DEVICE_STR);
    if(NULL == net_dev) {
        logger.log_message("missing constant: " + CONST_DEVICE_STR);
        return -1;
    }

    NetworkInterface net;
    if(SUCCESS != net.init(*net_dev, veh)) {
        logger.log_message("failed to open network interface");
        printf("failed to open network interface\n");

        return -1;
    }

    // add sequence number measurement
    std::string CONST_SEQUENCE_STR = CONST_SEQUENCE;
    std::string* m = veh->get_const(CONST_SEQUENCE_STR);
    if(NULL == m) {
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
        printf("have not received any telemetry, cannot execute program\n");
        logger.log_message("no telemetry received, cannot execute program");

        release_resources();
        return -1;
    }

    logger.log_message("engine program loaded");

    // wait until the clock is started
    int64_t curr_time;
    bool stopped = true;
    size_t first_event = 0;
    while(stopped) {
        if(FAILURE == cl.read_time(&curr_time, &stopped)) {
            logger.log_message("failed to read countdown clock");
            printf("failed to read countdown clock");

            release_resources();
            return -1;
        }

        // get rid of all the commands that happened prior to the current time
        // this stops us from starting the clock halfway through a program and executing all the beginning commands at once
        // NOTE: it's valid for a user to start a program when the clock is part way through that program
        // we do this in the loop so we don't have a delay after the clock starts to when we can start executing
        for(; first_event < events.size(); first_event++) {
            if(events[first_event].time < curr_time) {
                // this command has already passed, don't execute it
            } else {
                // stop at the first command that occurs after the current time
                // since commands are sorted, this is a valid assumption
                break;
            }
        }


        fflush(stdout);
        usleep(1000); // sleep 1 ms
    }

    logger.log_message("engine program started");

    if(first_event > 0) {
        logger.log_message("some events have already passed, those commands will not be executed");
        printf("some events have already passed, those commands will not be executed\n");
    }

    // get the current sequence number
    // TODO there could be an issue of sequence numbers if multiple commands execute at once
    //      e.g. the one we read is out of date (have a way to lock physical resources?)
    // ^^^ we lock the ENGINE_CONTROLLER_PROGRAM resource so no one else should be commanded
    // someone could potentially send a command (e.g. with ec_cmd) and that should cause a failed ack
    // ^^^ moved this after we lock everything, so this should be fine
    unsigned int seq_num;

    // execute all parsed commands
    ec_command_t cmd;
    long cmd_num;
    event_t e;
    for(size_t i = first_event; i < events.size(); i++) {
        e = events[i];

        if(FAILURE == cl.read_time(&curr_time, &stopped)) {
            logger.log_message("failed to read countdown clock");
            printf("failed to read countdown clock");

            release_resources();

            return -1;
        }

        if(stopped) {
            logger.log_message("clock was stopped, exiting program");
            printf("clock was stopped, exiting program\n");

            release_resources();

            return -1;
        }

        if(curr_time < e.time) {
            // we need to wait to execute this
            // NOTE: if the clock is reset during this time, we may miss the command
            usleep((e.time - curr_time) * 1000);
            i--;
            continue;
        }

        if(e.type == MESSAGE) {
            // message command
            logger.log_message(*(e.action.msg));
            delete e.action.msg;
            continue;
        } // otherwise it's a command

        // generate command
        cmd.control = e.action.cmd.control;
        cmd.state = e.action.cmd.state;

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

        // get the current sequence number
        if(SUCCESS != tlm.get_uint(meas, &seq_num)) {
            // this shouldn't happen
            printf("failed to get measurement: %s\n", meas.c_str());
            logger.log_message("failed to get sequence number measurement");

            release_resources();
            return -1;
        }

        cmd_num = seq_num + 1;
        cmd.seq_num = (uint32_t)cmd_num;

        // send the command over the network
        if(FAILURE == net.QueueUDPMessage((uint8_t*)&cmd, sizeof(cmd))) {
            printf("failed to queue message to network interface\n");
            logger.log_message("failed to queue message to network interface");

            return -1;
        }

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
                logger.log_message("failde to update telemetry");

                continue;
            } // otherwise success or timeout

            if(FAILURE == tlm.get_uint(meas, &seq_num)) {
                // this shouldn't happen
                printf("failed to get measurement: %s\n", meas.c_str());
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

    logger.log_message("engine program complete");
}
