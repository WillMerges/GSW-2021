#include <stdio.h>
#include <string.h>

#include <csignal>

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
    printf("%s failed to initialize", name);
    exit(-1);
}

void sighandler(int) {
    killed = true;

    // signal the shared memory control to force us to awaken
    // tlm.sighandler();
    tlm.sighandler();
}

int main(int argc, char* argv[]) {
    MsgLogger logger("json_convert");

    logger.log_message("Starting JSON conversion");

    std::string config_file = "";

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

    for (int i = 0; i < NUM_SIGNALS; i++) {
        signal(SIGNALS[i], sighandler);
    }

}