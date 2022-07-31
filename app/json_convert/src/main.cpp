#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

#include "lib/dls/dls.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/vcm/vcm.h"

#define NUM_SIGNALS 5
#define SERVER_IP localhost
#define SERVER_PORT 5000


using namespace dls;
using namespace vcm;

TelemetryViewer tlm;

bool killed = false;

const int SIGNALS[NUM_SIGNALS] = {SIGINT, SIGTERM, SIGSEGV, SIGFPE, SIGABRT};

void err_handle(std::string message, MsgLogger *logger) {
    std::string err_msg = "JSON_Convert: ";
    err_msg.append(message);

    logger->log_message(err_msg);
    std::cout << err_msg << std::endl;

    exit(-1);
}

void sighandler(int) {
    killed = true;
    tlm.sighandler();
}

std::string getJSONString(VCM *vcm, MsgLogger *logger, size_t max_size) {
    std::string jsonString = "{";

    for (std::string measurement : vcm->measurements) {
        measurement_info_t *meas_info = vcm->get_info(measurement);
        std::string *value = new std::string();
        if (FAILURE == tlm.get_str(meas_info, value)) {
            logger->log_message("Failed to get telemetry data");
            continue;
        }

        char pair_str[max_size];  // TODO: Causes warning about variable length.
                                  // Fix it?
        sprintf(pair_str, "'%s':%s,", measurement.c_str(), value->c_str());

        jsonString.append(pair_str);
    }

    jsonString.append("}");

    return jsonString;
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

    if (FAILURE == vcm->init()) err_handle("VCM failed to initialize", &logger);
    if (FAILURE == tlm.init()) err_handle("Telemetry Viewer failed to initialize", &logger);

    tlm.add_all();
    tlm.set_update_mode(TelemetryViewer::BLOCKING_UPDATE);

    unsigned int max_length = 0;
    size_t max_size = 0;
    int fields = 0;

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
        fields++;
    }

    max_size += (fields * 4) + 2; // Extra characters for JSON formatting

    // Setup UDP server
    int sockfd;
    char buffer[max_size];
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            err_handle("Socket File Descriptor initialization failed", &logger);
        }

    logger.log_message("JSON_Convert: Socket setup successful");


    bzero(&server_addr, sizeof(server_addr));
    bzero(&client_addr, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    if ((bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr))) < 0) {
        err_handle("Failed to bind", &logger);
    }


    while (1) {
        if (killed) {
            logger.log_message("The JSON conversion is unaliving itself now");
            exit(0);
        }
        
        std::string jsonString = getJSONString(vcm, &logger, max_size);
        std::cout << jsonString << std::endl;
    }

    return 0;
}