#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>
#include <unistd.h>
#include <csignal>
#include "lib/vcm/vcm.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/dls/dls.h"
#include "common/types.h"

// view telemetry values live
// run as val_view [-f path_to_config_file]

using namespace vcm;
using namespace shm;
using namespace dls;


TelemetryViewer tlm;

bool killed = false;

#define NUM_SIGNALS 5
int signals[NUM_SIGNALS] = {
                            SIGINT,
                            SIGTERM,
                            SIGSEGV,
                            SIGFPE,
                            SIGABRT
                        };


void sighandler(int) {
    killed = true;

    // signal the shared memory control to force us to awaken
    // tlm.sighandler();
    tlm.sighandler();
}


int main(int argc, char* argv[]) {
    MsgLogger logger("val_view");

    logger.log_message("starting val_view");

    std::string config_file = "";

    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "-f")) {
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

    std::shared_ptr<VCM> vcm;
    if(config_file.empty()) {
        vcm = std::make_shared<VCM>(); // use default config file
    } else {
        vcm = std::make_shared<VCM>(config_file); // use specified config file
    }

    if(FAILURE == vcm->init()) {
        logger.log_message("failed to initialize VCM");
        printf("failed to initialize VCM\n");
        exit(-1);
    }

    if(FAILURE == tlm.init(vcm)) {
        logger.log_message("failed to initialize telemetry viewer");
        printf("failed to initialize telemetry viewer\n");
        exit(-1);
    }

    // add signal handlers
    for(int i = 0; i < NUM_SIGNALS; i++) {
        signal(signals[i], sighandler);
    }

    tlm.add_all();
    tlm.set_update_mode(TelemetryViewer::BLOCKING_UPDATE);

    unsigned int max_length = 0;
    for(std::string it : vcm->measurements) {
        if(it.length() > max_length) {
            max_length = it.length();
        }
    }

    // clear the screen
    printf("\033[2J");

    measurement_info_t* m_info;
    std::string val;
    while(1) {
        if(killed) {
            exit(0);
        }

        for(std::string meas : vcm->measurements) {
            m_info = vcm->get_info(meas);

            printf("%s  ", meas.c_str());

            // print extra spaces
            for(size_t i = 0; i < max_length - meas.length(); i++) {
                printf(" ");
            }

            // if(tlm.updated(m_info)) {
            //     std::cout << "updated: ";
            // }

            if(FAILURE == tlm.get_str(m_info, &val)) {
                logger.log_message("failed to convert telemetry value");
                printf("failed to convert telemetry value\n");

                std::cout << "ERR\n";
                continue; // keep on going
            } else {
                std::cout << val << "\n";
            }

        }

        // update telemetry
        if(FAILURE == tlm.update()) {
            logger.log_message("failed to update telemetry");
            printf("failed to update telemetry\n");
            // ignore and continue
        } else {
            // clear the screen
            printf("\033[2J");
        }

        usleep(75000);
    }
}
