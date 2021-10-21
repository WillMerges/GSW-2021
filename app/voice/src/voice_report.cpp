#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>
#include <unistd.h>
#include <float.h>
#include <csignal>
#include "lib/vcm/vcm.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/dls/dls.h"
#include "lib/convert/convert.h"
#include "common/types.h"

// use text to speach to report altitude and final GPS location
// usage: ./voice_report [-f config_file]
//        the -f argument specifies the config file location, or the default is used

// measurement names, as in VCM config file
// TODO make these arguments
#define LAT "GPS_LAT"
#define LONG "GPS_LONG"
#define ALT "ALT" // assumes it's an altitude AGL, TODO make this work for GPS altitude with a base altitude

// how many feet in between each readout
#define ALT_RESOLUTION 500

// how many times altitude decreases before it's considered apogee
#define ALT_NUM_DECR 3

// how many seconds to wait before reporting last location
#define LOCATION_TIMEOUT 10

// if altitude stays withing +/- resolution for LANDING_NUM_PACKETS the last location will be read
#define LANDING_ALT_RESOLUTION 5 // in whatever units ALT measurement is
#define LANDING_NUM_PACKETS 5

// text-to-speach bash command, assuming executing from GSW_HOME
#define TTS_COMMAND "/app/voice/speak.sh"

using namespace vcm;
using namespace shm;
using namespace dls;
using namespace convert;

// GSW_HOME environment variable
std::string gsw_home;

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

    // shut the compiler up
    int garbage = signum;
    garbage = garbage + 1;
}


void report_alt(float alt) {
    char buff[1024];
    snprintf(buff, 1024, "%.0f feet", alt);
    std::string cmd = gsw_home;
    cmd += TTS_COMMAND;
    cmd += " \"";
    cmd += buff;
    cmd += "\"";
    std::cout << cmd.c_str() << "\n";
    system(cmd.c_str());
}

void report_apogee(float alt) {
    char buff[1024];
    snprintf(buff, 1024, "apogee reached at %.0f feet", alt);
    std::string cmd = gsw_home;
    cmd += TTS_COMMAND;
    cmd += " \"";
    cmd += buff;
    cmd += "\"";
    std::cout << cmd.c_str() << "\n";
    system(cmd.c_str());
}

void report_landing(float lat, float lon, float alt) {
    char buff[1024];
    snprintf(buff, 1024, "landed at %f, %f, %.0f feet", lat, lon, alt);
    std::string cmd = gsw_home;
    cmd += TTS_COMMAND;
    cmd += " \"";
    cmd += buff;
    cmd += "\"";
    std::cout << cmd.c_str() << "\n";
    system(cmd.c_str());
}

void report_loc(float lat, float lon, float alt) {
    char buff[1024];
    snprintf(buff, 1024, "last known position was %f, %f, %.0f feet", lat, lon, alt);
    std::string cmd = gsw_home;
    cmd += TTS_COMMAND;
    cmd += " \"";
    cmd += buff;
    cmd += "\"";
    std::cout << cmd.c_str() << "\n";
    system(cmd.c_str());
}

int main(int argc, char* argv[]) {
    MsgLogger logger("voice_report");

    logger.log_message("starting voice_report");

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

    char* env = getenv("GSW_HOME");
    if(env == NULL) {
        logger.log_message("GSW_HOME environment variable not set!");
        throw new std::runtime_error("Environment error in VCM");
    }
    gsw_home = env;

    VCM* vcm;
    if(config_file == "") {
        vcm = new VCM(); // use default config file
    } else {
        vcm = new VCM(gsw_home + "/" + config_file); // use specified config file
    }

    if(FAILURE == vcm->init()) {
        logger.log_message("failed to initialize VCM");
        printf("failed to initialize VCM\n");
        exit(-1);
    }

    TelemetryViewer tlm;
    if(FAILURE == tlm.init(vcm)) {
        logger.log_message("failed to initialize telemetry viewer");
        printf("failed to initialize telemetry viewer\n");
        exit(-1);
    }

    std::string gps_lat = LAT;
    std::string gps_long = LONG;
    std::string gps_alt = ALT;

    if(FAILURE == tlm.add(gps_lat)) {
        logger.log_message("failed to add " + gps_lat + " to telemetry viewer");
        printf("failed to add %s to telemetry viewer", gps_lat.c_str());
        exit(-1);
    }
    if(FAILURE == tlm.add(gps_long)) {
        logger.log_message("failed to add " + gps_long + " to telemetry viewer");
        printf("failed to add %s to telemetry viewer", gps_long.c_str());
        exit(-1);
    }
    if(FAILURE == tlm.add(gps_alt)) {
        logger.log_message("failed to add " + gps_alt + " to telemetry viewer");
        printf("failed to add %s to telemetry viewer", gps_alt.c_str());
        exit(-1);
    }

    tlm.set_update_mode(TelemetryViewer::NONBLOCKING_UPDATE);

    measurement_info_t* lat_meas = vcm->get_info(gps_lat);
    measurement_info_t* long_meas = vcm->get_info(gps_long);
    measurement_info_t* alt_meas = vcm->get_info(gps_alt);

    if(!lat_meas || !long_meas || !alt_meas) {
        logger.log_message("Missing measurement, cannot continue");
        printf("Missing measurement, cannot continue");
        exit(-1);
    }

    float lat;
    float lon;
    float alt;

    // the next altitude to readout at
    float next_alt = ALT_RESOLUTION;

    // whether we're ascending or descending
    uint8_t ascending = 1;

    float max_alt = 0;
    int num_decreases = 0;
    float last_alt = 0;
    int started = 0; // if we've left the pad

    clock_t t;
    t = clock();
    clock_t elapsed;

    while(1) {
        if(killed) {
            exit(0);
        }

        // update telemetry
        if(SUCCESS != tlm.update()) {
            if(started) {
                // only check fot timeout if we've gotten any packets, dont want to prematurely report
                elapsed = clock() - t;
                if((((double)elapsed) / CLOCKS_PER_SEC) > LOCATION_TIMEOUT) {
                    report_loc(lat, lon, alt);
                    t = clock();
                }
            }
            continue;
        } else {
            // if we've gotten any packet, we've taken off
            // TODO maybe check for leaving the pad?
            started = 1;

            // restart the clock
            t = clock();
        }

        if(FAILURE == tlm.get_float(lat_meas, &lat)) {
            logger.log_message("failed to get latitude telemetry");
            continue;
        }

        if(FAILURE == tlm.get_float(long_meas, &lon)) {
            logger.log_message("failed to get longitude telemetry");
            continue;
        }

        if(FAILURE == tlm.get_float(alt_meas, &alt)) {
            logger.log_message("failed to get altitude telemetry");
            continue;
        }

        if(ascending) {
            if(alt > next_alt) {
                report_alt(next_alt);
                next_alt += ALT_RESOLUTION;
            }

            if(alt > max_alt) {
                max_alt = alt;
            } else {
                num_decreases++;
                if(num_decreases > ALT_NUM_DECR) {
                    ascending = 0;
                    report_apogee(max_alt);
                    last_alt = alt;
                }
            }
        } else {
            if(alt < next_alt) {
                report_alt(next_alt);
                next_alt -= ALT_RESOLUTION;
            }

            if(alt > last_alt - LANDING_ALT_RESOLUTION && alt < last_alt + LANDING_ALT_RESOLUTION) {
                num_decreases++;
                if(num_decreases == LANDING_NUM_PACKETS) {
                    report_landing(lat, lon, alt);
                }
            }
        }
    }
}
