/*******************************************************************************
* Name: main.cpp
*
* Purpose: Logging Control tool
*          Allows telemetry logging to disk to be disabled or enabled
*
*          Usage ./log_ctrl [enable | disable]
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
#include "lib/dls/dls.h"
#include "common/types.h"
#include <string.h>

using namespace dls;



int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("usage: ./log_ctrl [enable | disable]\n");
        return -1;
    }

    MsgLogger logger("LOG_CTRL");

    DlShm shm;

    if(SUCCESS != shm.init()) {
        logger.log_message("failed to initialized data logger shared memory");
        return -1;
    }

    if(SUCCESS != shm.attach()) {
        logger.log_message("failed to attach to data logger shared memory");
        return -1;
    }

    std::string arg = argv[1];

    if(arg == "enable") {
        if(SUCCESS != shm.set_logging(true)) {
            logger.log_message("failed to enabled telemetry logging");
            return -1;
        }

        logger.log_message("enabled telemetry logging");
    } else if(arg == "disable") {
        if(SUCCESS != shm.set_logging(false)) {
            logger.log_message("failed to disable telemetry logging");
            return -1;
        }

        logger.log_message("disabled telemetry logging");
    } else {
        printf("usage: ./log_ctrl [enable | disable]\n");
        return -1;
    }

    if(SUCCESS != shm.detach()) {
        logger.log_message("failed to detach from data logger shared memory");
        return -1;
    }
}
