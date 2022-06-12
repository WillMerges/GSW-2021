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
#include "lib/convert/convert.h"
#include "common/types.h"

// print measurements separated by spaces
// run as printmeas [-f path_to_config_file] [space-separated list of measurement to print] [-t to print timestamp before measurements | -g to print in feedgnuplot format]
// if config file path not specified will use the default

using namespace vcm;
using namespace shm;
using namespace dls;
using namespace convert;


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

    tlm.sighandler();
}


static struct timespec tp;
static unsigned long long start_time; //ms

inline RetType init_clock() {
    if(clock_gettime(CLOCK_MONOTONIC, &tp) != 0) {
        return FAILURE;
    }

    start_time = (tp.tv_sec * 1000) + (tp.tv_nsec / 1000000);
    return SUCCESS;
}


inline void print_timestamp() {
    tp.tv_sec = 0;
    tp.tv_nsec = 0;
    if(clock_gettime(CLOCK_MONOTONIC, &tp) != 0) {
        printf("error");
        return;
    }

    unsigned long long millis = (tp.tv_sec * 1000) + (tp.tv_nsec / 1000000);
    printf("%llu", millis - start_time);
}


int main(int argc, char* argv[]) {
    MsgLogger logger("print_meas");

    logger.log_message("starting print_meas");

    // add signal handlers
    for(int i = 0; i < NUM_SIGNALS; i++) {
        signal(signals[i], sighandler);
    }

    std::string config_file = "";

    // parse args
    size_t index = 1;
    if(!strcmp(argv[1], "-f")) {
        if(argc < 3) {
            logger.log_message("Must specify a path to the config file after using the -f option");
            printf("Must specify a path to the config file after using the -f option\n");
            return -1;
        } else {
            config_file = argv[2];
            index = 3;
        }
    }

    bool print_timestamps = false;
    bool print_replots = false;

    if(!strcmp(argv[argc - 1], "-t")) {
        if(SUCCESS != init_clock()) {
            logger.log_message("Failed to initialize clock");
            printf("Failed to initialize clock\n");
            return -1;
        }

        print_timestamps = true;
        argc--;
    } else if(!strcmp(argv[argc - 1], "-g")) {
       print_replots = true;
       print_timestamps = true;
       argc--;
    }


    std::vector<std::string> measurements;
    for(int i = index; i < argc; i++) {
        measurements.push_back(argv[i]);
    }

    // create VCM
    VCM* vcm;
    if(config_file == "") {
        vcm = new VCM(); // use default config file
    } else {
        vcm = new VCM(config_file); // use specified config file
    }

    // init VCM
    if(FAILURE == vcm->init()) {
        logger.log_message("failed to initialize VCM");
        printf("failed to initialize VCM\n");
        return -1;
    }

    // init telemetry viewer
    if(FAILURE == tlm.init(vcm)) {
        logger.log_message("failed to initialize telemetry viewer");
        printf("failed to initialize telemetry viewer\n");
        return -1;
    }

    // add measurements to telemetry viewer
    // also track info blocks for each measurements
    std::vector<measurement_info_t*> meas_infos;
    measurement_info_t* info;
    for(int i = measurements.size()-1; i >= 0; i--) {
        info = vcm->get_info(measurements[i]);
        if(info == NULL) {
            logger.log_message("measurement does not exist: " + measurements[i]);
            printf("measurement does not exist: %s\n", measurements[i].c_str());
            return -1;
        }

        meas_infos.push_back(info);

        if(FAILURE == tlm.add(measurements[i])) {
            logger.log_message("failed to add " + measurements[i] + " to telemetry viewer");
            printf("failed to add %s to telemetry viewer\n", measurements[i].c_str());
        }
    }

    tlm.set_update_mode(TelemetryViewer::BLOCKING_UPDATE);

    std::string str;

    while(1) {
        if(killed) {
            exit(0);
        }

        // update telemetry
        if(FAILURE == tlm.update()) {
            logger.log_message("failed to update telemetry");
            printf("failed to update telemetry\n");
            // ignore and continue
            continue;
        }

        if(print_timestamps) {
            print_timestamp();
            printf(" ");
        }

        for(size_t i = 0; i < meas_infos.size(); i++) {
            if(FAILURE == tlm.get_str(info, &str)) {
                // ignore and try updating again
                continue;
            }

            printf("%s", str.c_str());

            if(i != meas_infos.size() - 1) {
                printf(" ");
            }
        }

        if(print_replots) {
            printf("\nreplot\n");
            fflush(stdout);
        } else {
            printf("\n");
            fflush(stdout);
        }

        usleep(1000); // sleep for 1 ms
    }
}
