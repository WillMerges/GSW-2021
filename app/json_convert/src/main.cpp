#include <cstdio>
#include <cstring>

#include <csignal>
#include <iostream>

#include "lib/dls/dls.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/vcm/vcm.h"

using namespace dls;
using namespace vcm;

TelemetryViewer tlm;

bool killed = false;

#define NUM_SIGNALS 5
const int SIGNALS[NUM_SIGNALS] = {SIGINT, SIGTERM, SIGSEGV, SIGFPE, SIGABRT};

void failed_start(std::string name, MsgLogger *logger) {
    logger->log_message("Failed to initialize " + name);
    std::cout << name << " failed to initialize";

    exit(-1);
}

void sighandler(int) {
    killed = true;
    tlm.sighandler();
}

int main(int argc, char* argv[]) {
    MsgLogger logger("json_convert");

    logger.log_message("Starting JSON conversion");

    std::string config_file = "";

    for (int i = 0; i < NUM_SIGNALS; i++) {
        signal(SIGNALS[i], sighandler);
    }

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-f")) {
            if (i + 1 > argc) {
                logger.log_message(
                    "Must specify a path to the config file after using the -f "
                    "option");
                printf(
                    "Must specify a path to the config file after using the -f "
                    "option\n");
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

    if (config_file == "") {
        vcm = new VCM();
    } else {
        vcm = new VCM(config_file);
    }

    if (FAILURE == vcm->init()) failed_start("VCM", &logger);
    if (FAILURE == tlm.init()) failed_start("telemetry viewer", &logger);

    tlm.add_all();
    tlm.set_update_mode(TelemetryViewer::BLOCKING_UPDATE);

    unsigned int max_length = 0;
    size_t max_size = 0;

    // Sets the max sizes
    for (std::string it : vcm->measurements) {
        if (it.length() > max_length) {
            max_length = it.length();
        }

        measurement_info_t* info = vcm->get_info(it);
        // assuming info isn't NULL since it's in the vcm list
        if (info->size > max_size) {
            max_size = info->size;
        }
    }

    while (1) {
        if (killed) {
            logger.log_message("The JSON conversion is unaliving itself now");
            exit(0);
        }
        
        std::string jsonString = "{";

        for (std::string measurement : vcm->measurements) {
            measurement_info_t *meas_info = vcm->get_info(measurement);
            std::string *value = new std::string();
            if (FAILURE == tlm.get_str(meas_info, value)) {
                logger.log_message("Failed to get telemetry data");
                continue;
            }


            char pair_str[max_size]; // TODO: Causes warning about variable length. Fix it?
            sprintf(pair_str, "'%s':%s,", measurement.c_str(), value->c_str());
            jsonString.append(pair_str);
        }

        jsonString.append("}");

        // TODO: Send it
        std::cout << jsonString << std::endl;

        jsonString = "";
    }

    return 0;
}