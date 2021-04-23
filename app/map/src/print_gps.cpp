#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>
#include <unistd.h>
#include "lib/vcm/vcm.h"
#include "lib/shm/shm.h"
#include "lib/dls/dls.h"
#include "lib/convert/convert.h"
#include "common/types.h"

// view telemetry memory live
// run as printgps [-f path_to_config_file]
// if config file path not specified will use the default

// looks for measurements names GPS_LAT, GPS_LONG, and GPS_ALT
// prints them to stdout separated by spaces in that order

#define GPS_LAT "GPS_LAT"
#define GPS_LONG "GPS_LONG"
#define GPS_ALT "GPS_ALT"

using namespace vcm;
using namespace shm;
using namespace dls;
using namespace convert;

int main(int argc, char* argv[]) {
    MsgLogger logger("print_gps");

    logger.log_message("starting print_gps");

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

    if(FAILURE == attach_to_shm(vcm)) {
        logger.log_message("unable to attach mem_view process to shared memory");
        printf("unable to attach mem_view process to shared memory\n");
        return FAILURE;
    }

    unsigned char* buff = new unsigned char[vcm->packet_size];
    memset((void*)buff, 0, vcm->packet_size); // zero the buffer

    measurement_info_t* lat_meas = vcm->get_info(GPS_LAT);
    measurement_info_t* long_meas = vcm->get_info(GPS_LONG);
    measurement_info_t* alt_meas = vcm->get_info(GPS_ALT);

    if(!lat_meas || !long_meas || !alt_meas) {
        logger.log_message("Missing GPS measurement");
        exit(-1);
    }

    std::string lat;
    std::string lon;
    std::string alt;

    while(1) {
        // read from shared memoery
        if(FAILURE == read_from_shm_block((void*)buff, vcm->packet_size)) {
            logger.log_message("failed to read from shared memory");
            // ignore and continue
            continue;
        }

        if(FAILURE == convert_str(vcm, lat_meas, buff, &lat)) {
            continue;
        }

        if(FAILURE == convert_str(vcm, long_meas, buff, &lon)) {
            continue;
        }

        if(FAILURE == convert_str(vcm, alt_meas, buff, &alt)) {
            continue;
        }

        printf("%s %s %s\n", lat.c_str(), lon.c_str(), alt.c_str());

        usleep(1000); // sleep for 1 ms
    }
}
