#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include "lib/vcm/vcm.h"
#include "lib/shm/shm.h"
#include "lib/dls/dls.h"
#include "lib/telemetry/TelemetryShm.h"
#include "common/types.h"
#include "lib/nm/NmShm.h"
#include "lib/clock/clock.h"
#include "lib/vlock/vlock.h"

// run as shmctl -on or shmctl -off to create and destroy shared memory
// option -f argument to specify VCM config file (current default used otherwise)
// use as shmctl (-on | -off) [-f path_to_config_file]

using namespace vcm;
using namespace shm;
using namespace dls;
using namespace countdown_clock;

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

    CountdownClock cl;
    if(cl.init() == FAILURE) {
        printf("failed to initialize countdown clock\n");
        logger.log_message("failed to initialize countdown clock");
        return FAILURE;
    }

    NmShm nm_shm;
    if(vcm->num_net_devices > 0) {
        if(nm_shm.init(vcm->num_net_devices) == FAILURE) {
            printf("failed to initialize network manager shm controller\n");
            logger.log_message("failed to initialize network manager shm controller");
            return FAILURE;
        }
    }

    TelemetryShm tlm_shm;
    if(tlm_shm.init(vcm) == FAILURE) {
        printf("failed to initialize telemetry shm controller\n");
        logger.log_message("failed to initialize telemetry shm controller");
        return FAILURE;
    }

    RetType ret = SUCCESS;
    if(on) {
        printf("creating shared memory\n");
        logger.log_message("creating shared memory");

        if(FAILURE == tlm_shm.create()) {
            printf("failed to create telemetry shared memory\n");
            logger.log_message("failed to create telemetry shared memory");
            ret = FAILURE;
        } else {
            printf("created telemetry shared memory\n");
            logger.log_message("created telemetry shared memory");
        }

        if(vcm->num_net_devices > 0) {
            if(FAILURE == nm_shm.create()) {
                printf("failed to create network manager shared memory\n");
                logger.log_message("failed to create network manager shared memory");
                ret = FAILURE;
            } else {
                printf("created network manager shared memory\n");
                logger.log_message("created network manager shared memory");
            }
        }

        if(FAILURE == cl.create()) {
            printf("failed to create countdown clock shared memory\n");
            logger.log_message("failed to create countdown clock shared memory");
            ret = FAILURE;
        } else {
            printf("created countdown clock shared memory\n");
            logger.log_message("created countdown clock shared memory");
        }

        if(FAILURE == vlock::create_shm()) {
            printf("failed to create vlock shared memory\n");
            logger.log_message("failed to create vlock shared memory");
            ret = FAILURE;
        } else {
            printf("created vlock shared memory\n");
            logger.log_message("created vlock shared memory");
        }

        return ret;
    } else if(off) {
        printf("destroying shared memory\n");
        logger.log_message("destroying shared memory");

        if(FAILURE == tlm_shm.attach()) {
            printf("telemetry shared memory not created, nothing to destroy\n");
            logger.log_message("telemetry shared memory not created, nothing to destroy");
            ret = FAILURE;
        } else {
            if(FAILURE == tlm_shm.destroy()) {
                printf("failed to destroy telemetry shared memory\n");
                logger.log_message("failed to destroy telemetry shared memory");
                ret = FAILURE;
            }
        }

        if(vcm->num_net_devices > 0) {
            if(FAILURE == nm_shm.attach()) {
                printf("network manager shared memory not created, nothing to destroy\n");
                logger.log_message("network manager shared memory not created, nothing to destroy");
                ret = FAILURE;
            } else {
                if(FAILURE == nm_shm.destroy()) {
                    printf("failed to destroy network manager shared memory\n");
                    logger.log_message("failed to destroy network manager shared memory");
                    ret = FAILURE;
                }
            }
        }

        if(FAILURE == cl.open()) {
            printf("countdown clock shared memory not created, nothing to destroy\n");
            logger.log_message("countdown clock shared memory not created, nothing to destroy");
            ret = FAILURE;
        } else {
            if(FAILURE == cl.destroy()) {
                printf("failed to destroy countdown clock shared memory\n");
                logger.log_message("failed to destroy countdown clock shared memory");
                ret = FAILURE;
            }
        }

        if(FAILURE == vlock::destroy_shm()) {
            printf("failed to destroy vlock shared memory\n");
            logger.log_message("failed to destroy vlock shared memory");
            ret = FAILURE;
        }

        return ret;
    }
}
