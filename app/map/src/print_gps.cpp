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

// view telemetry memory live
// run as printgps [-f path_to_config_file]
// if config file path not specified will use the default

// looks for measurements named GPS_LAT, GPS_LONG, and GPS_ALT
// prints them to stdout separated by spaces in that order

// TODO perhaps make the measurement names command line arguments?

#define GPS_LAT "GPS_LAT"
#define GPS_LONG "GPS_LONG"
#define GPS_ALT "GPS_ALT"

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

void sighandler(int signum) {
    killed = true;

    tlm.sighandler();

    // shut the compiler up
    int garbage = signum;
    garbage = garbage + 1;
}


int main(int argc, char* argv[]) {
    MsgLogger logger("print_gps");

    logger.log_message("starting print_gps");

    // add signal handlers
    for(int i = 0; i < NUM_SIGNALS; i++) {
        signal(signals[i], sighandler);
    }

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

    VCM* vcm;
    if(config_file == "") {
        vcm = new VCM(); // use default config file
    } else {
        vcm = new VCM(config_file); // use specified config file
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

    std::string gps_lat = GPS_LAT;
    std::string gps_long = GPS_LONG;
    std::string gps_alt = GPS_ALT;

    if(FAILURE == tlm.add(gps_lat)) {
        logger.log_message("failed to add " + gps_lat + " to telemetry viewer");
        exit(-1);
    }
    if(FAILURE == tlm.add(gps_long)) {
        logger.log_message("failed to add " + gps_long + " to telemetry viewer");
        exit(-1);
    }
    if(FAILURE == tlm.add(gps_alt)) {
        logger.log_message("failed to add " + gps_alt + " to telemetry viewer");
        exit(-1);
    }

    tlm.set_update_mode(TelemetryViewer::BLOCKING_UPDATE);

    measurement_info_t* lat_meas = vcm->get_info(gps_lat);
    measurement_info_t* long_meas = vcm->get_info(gps_long);
    measurement_info_t* alt_meas = vcm->get_info(gps_alt);

    if(!lat_meas || !long_meas || !alt_meas) {
        logger.log_message("Missing GPS measurement");
        exit(-1);
    }

    std::string lat;
    std::string lon;
    std::string alt;

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

        if(FAILURE == tlm.get_str(lat_meas, &lat)) {
            continue;
        }

        if(FAILURE == tlm.get_str(long_meas, &lon)) {
            continue;
        }

        if(FAILURE == tlm.get_str(alt_meas, &alt)) {
            continue;
        }

        printf("%s %s %s\n", lat.c_str(), lon.c_str(), alt.c_str());

        usleep(1000); // sleep for 1 ms
    }
}
