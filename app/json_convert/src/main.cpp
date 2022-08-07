/**
 *  Name: main.cpp
 *  
 *  Purpose: Takes telemetry data and forwards it as a JSON through UDP
 * 
 *  Author: Aaron Chan
 * 
 *  Usage: ./json_convert [SERVER_PORT] [CLIENT_PORT]
 *      - Port specification in CLI is optional
 *      - Port in config file takes priority
 *      - If no port is specified by either, default port will be set to 5000 
 */


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
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define CLIENT_PORT 5100


using namespace dls;
using namespace vcm;

TelemetryViewer tlm;

bool killed = false;

const int SIGNALS[NUM_SIGNALS] = {SIGINT, SIGTERM, SIGSEGV, SIGFPE, SIGABRT};
const std::string logger_name = "json_convert";

/**
 * Handles errors and provides error message to user and logger
 */
void err_handle(std::string err_msg) {
    MsgLogger logger(logger_name);

    logger.log_message(err_msg);
    std::cout << err_msg << std::endl;

    exit(-1);
}

/**
 * Handles signals
 */
void sighandler(int) {
    killed = true;
    tlm.sighandler();
}

/**
 * Fetches data and converts it into a JSON string
 */
std::string getJSONString(VCM *vcm, size_t max_size) {
    tlm.update();

    MsgLogger logger(logger_name);
    std::string jsonString = "{";


    for (std::string measurement : vcm->measurements) {
        measurement_info_t *meas_info = vcm->get_info(measurement);
        std::string *value = new std::string();

        if (FAILURE == tlm.get_str(meas_info, value)) {
            logger.log_message("Failed to get telemetry data");
            continue;
        }

        char pair_str[max_size];  // TODO: Causes warning about variable length.
                                  // Fix it?
        sprintf(pair_str, "\"%s\":%s,", measurement.c_str(), value->c_str());

        jsonString.append(pair_str);
    }

    jsonString.append("}");

    return jsonString;
}

/**
 * Main function for setup and sending through UDP
 */
int main(int argc, char* argv[]) {
    MsgLogger logger(logger_name);

    logger.log_message("Starting JSON conversion");

    std::string config_file = "";

    for (int i = 0; i < NUM_SIGNALS; i++) {
        signal(SIGNALS[i], sighandler);
    }

    VCM* vcm;

    if (config_file == "") {
        vcm = new VCM();
    } else {
        vcm = new VCM(config_file);
    }

    if (FAILURE == vcm->init()) err_handle("VCM failed to initialize");
    if (FAILURE == tlm.init()) err_handle("Telemetry Viewer failed to initialize");

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
        err_handle("Socket File Descriptor initialization failed");
    }

    logger.log_message("Socket setup successful");

    std::string config_server_port = "JSON_SERVER_PORT";
    std::string* server_port_str = vcm->get_const(config_server_port);
    if (server_port_str == NULL && argc > 1) {
        server_port_str = new std::string(argv[1]);
    }

    std::string config_client_port = "JSON_PORT";
    std::string* client_port_str = vcm->get_const(config_client_port);
    if (client_port_str == NULL && argc > 2) {
        client_port_str = new std::string(argv[2]);
    }

    int server_port = server_port_str != NULL ? std::stoi(*server_port_str) : SERVER_PORT;
    int client_port = client_port_str != NULL ? std::stoi(*client_port_str) : CLIENT_PORT;
    std::cout << "Server Port: " <<  server_port << " Client Port: " << client_port << std::endl;

    bzero(&server_addr, sizeof(server_addr));
    bzero(&client_addr, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(client_port);

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int));

    if ((bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr))) < 0) err_handle("Failed to bind");
    
    while (1) {
        if (killed) {
            logger.log_message("Unaliving process");
            exit(0);
        }   

        std::string jsonString = getJSONString(vcm, max_size);

        // Send data to client
        sendto(sockfd, jsonString.c_str(), jsonString.length(), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    }

    return 0;
}