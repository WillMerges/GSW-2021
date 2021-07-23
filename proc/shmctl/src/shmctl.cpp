#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include "lib/vcm/vcm.h"
#include "lib/shm/shm.h"
#include "lib/dls/dls.h"
#include "lib/telemetry/TelemetryShm.h"
#include "common/types.h"

// run as shmctl -on or shmctl -off to create and destroy shared memory
// option -f argument to specify VCM config file (current default used otherwise)
// use as shmctl (-on | -off) [-f path_to_config_file]

using namespace vcm;
using namespace shm;
using namespace dls;

bool on = false;
bool off = false;

int main(int argc, char* argv[]) {
    MsgLogger logger("SHMCTL");

    std::string config_file = "";

    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "-on") && !off) {
            on = true;
        } else if(!strcmp(argv[i], "-off") && !on) {
            off = true;
        } else if(!strcmp(argv[i], "-f")) {
            if(i + 1 > argc) {
                logger.log_message("Must specify a path to the config file after using the -f option");
                printf("Must specify a path to the config file after using the -f option\n");
                return -1;
            } else {
                config_file = argv[++i];
            }
        } else {
            std::string msg = "Invalid argument: ";
            msg += argv[i];
            logger.log_message(msg.c_str());
            printf("Invalid argument: %s\n", argv[i]);
            return -1;
        }
    }

    VCM* vcm;
    try {
        if(config_file == "") {
            vcm = new VCM(); // use default config file
        } else {
            vcm = new VCM(config_file); // use specified config file
        }
    } catch (const std::runtime_error& e) {
        std::cout << e.what() << '\n';
        exit(-1);
    }

    if(vcm->init() == FAILURE) {
        printf("failed to initialize vehicle configuration manager\n");
        logger.log_message("failed to initialize vehicle configuration manager");
        return FAILURE;
    }

    TelemetryShm mem;
    if(mem.init(vcm) == FAILURE) {
        printf("failed to initialize telemetry shm controller\n");
        logger.log_message("failed to initialize telemetry shm controller");
        return FAILURE;
    }

    if(on) {
        printf("creating shared memory\n");
        logger.log_message("creating shared memory");
        if(FAILURE == mem.create()) {
            printf("Failed to create shared memory\n");
            logger.log_message("Failed to create shared memory");
            return FAILURE;
        }
        return SUCCESS;
    } else if(off) {
        printf("destroying shared memory\n");
        logger.log_message("destroying shared memory");
        if(FAILURE == mem.open()) {
            printf("Shared memory not created, nothing to destroy\n");
            logger.log_message("Shared memory not created, nothing to destroy");
        }
        if(FAILURE == mem.destroy()) {
            printf("Failed to destroy shared memory\n");
            logger.log_message("Failed to destroy shared memory");
            return FAILURE;
        }
        return SUCCESS;
    }
}
