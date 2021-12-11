/*******************************************************************************
* Name: main.cpp
*
* Purpose: Countdown clock viewer application
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
#include "lib/clock/clock.h"
#include "lib/dls/dls.h"
#include <stdio.h>

using namespace countdown_clock;
using namespace dls;

int main() {
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

    int64_t time;
    while(1) {
        if(FAILURE != cl.read_time(&time)) {
            printf("\033[2J"); // clear the screen
            // printf("time: %ld\n", time);
        }
    }
}
