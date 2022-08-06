/*******************************************************************************
* Name: main.cpp
*
* Purpose: Countdown clock viewer application
*
* Author: Will Merges
*
*
* Usage: ./clock_ctrl [set N]
*        Set the clock to 'N' milliseconds from T-0
*
* Usage: ./clock_ctrl [hold N]
*        Set a hold at 'N' milliseconds from T-0
*
* Usage: ./clock_ctrl [start]
*        Start the clock if it is stopped
*
* Usage: ./clock_ctrl [stop]
*        Stop the clock
*
* Usage: ./clock_ctrl [release]
*        Release the hold on the clock if there is one
*
*
* RIT Launch Initiative
*******************************************************************************/
#include "lib/clock/clock.h"
#include "lib/dls/dls.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "lib/dev/xbee/xbee.h"
#include "lib/dev/serial/serial.h"

using namespace countdown_clock;
using namespace dls;

int main(int argc, char* argv[]) {
    MsgLogger logger("CLOCK_VIEW", "main");

    CountdownClock cl;

    if(FAILURE == cl.init()) {
        logger.log_message("Failed to init clock");
        printf("Failed to init clock\n");

        return -1;
    }

    if(FAILURE == cl.open()) {
        logger.log_message("Failed to open clock");
        printf("Failed to open clock\n");

        return -1;
    }

    clock_cmd_t cmd;
    cmd.cmd = NUM_CLOCK_CMDS;

    if(argc < 2) {
        printf("Invalid arguments\n");
        logger.log_message("Invalid arguments");

        return -1;
    }

    if(!strcmp(argv[1], "start")) {
        cmd.cmd = START_CLOCK;
    } else if(!strcmp(argv[1], "stop")) {
        cmd.cmd = STOP_CLOCK;
    } else if(!strcmp(argv[1], "release")) {
        cmd.cmd = RELEASE_HOLD_CLOCK;
    }

    if(cmd.cmd != NUM_CLOCK_CMDS) {
        if(FAILURE == cl.parse_cmd(&cmd)) {
            printf("Failed to execute clock command\n");
            logger.log_message("Failed to execute clock command");

            return -1;
        } else {
            return 0;
        }
    }

    // need an extra arg for setting the clock/hold time
    if(argc < 3) {
        printf("Invalid arguments\n");
        logger.log_message("Invalid arguments");

        return -1;
    }

    cmd.arg = strtol(argv[2], NULL, 10);
    if(cmd.arg == 0 && errno == EINVAL) {
        printf("Could not convert time argument\n");
        logger.log_message("Could not convert eim argument");

        return -1;
    }

    if(!strcmp(argv[1], "set")) {
        cmd.cmd = SET_CLOCK;
    } else if(!strcmp(argv[1], "hold")) {
        cmd.cmd = SET_HOLD_CLOCK;
    }

    if(cmd.cmd != NUM_CLOCK_CMDS) {
        if(FAILURE == cl.parse_cmd(&cmd)) {
            printf("Failed to execute clock command\n");
            logger.log_message("Failed to execute clock command");

            return -1;
        } else {
            return 0;
        }
    }

    printf("Invalid arguments\n");
    logger.log_message("Invalid arguments");
    return -1;
}
