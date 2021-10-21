// forwards packets from shared mem. to InfluxDB using UDP line protocol
// run as ./fwd_influx [-f config_file]
// if config file not specified with -f option, uses the default location
//
// if there is a measurement called "UPTIME" it will be used as a timestamp
// "UPTIME" is expected to be in units of milliseconds
// otherwise the InfluxDB server clock will be used as the timestamp

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <csignal>
#include "lib/vcm/vcm.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/dls/dls.h"
#include "lib/convert/convert.h"
#include "common/types.h"

// TODO
// remove printfs? or add verbose mode

using namespace vcm;
using namespace shm;
using namespace dls;
using namespace convert;

#define INFLUXDB_UDP_PORT 8089
#define INFLUXDB_ADDR "127.0.0.1"

#define NANOSEC_PER_MILLISEC (10^6)

int sockfd;
unsigned char sock_open = 0;
bool killed = false;

void sighandler(int signum) {
    if(sock_open) {
        close(sockfd);
    }

    killed = true;

    // shut the compiler up
    int garbage = signum;
    garbage = garbage + 1;
}

int main(int argc, char* argv[]) {
    MsgLogger logger("DB_FWD");

    logger.log_message("starting database forwarding");

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

    // add signal handlers to close the socket if opened
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGFPE, sighandler);
    signal(SIGABRT, sighandler);

    // set up the UDP socket
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
       logger.log_message("socket creation failed");
       printf("socket creation failed\n");
       return FAILURE;
    }
    sock_open = 1;

    // define influxdb server endpoint
    struct sockaddr_in servaddr;

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(INFLUXDB_UDP_PORT);
    servaddr.sin_addr.s_addr = inet_addr(INFLUXDB_ADDR);

    TelemetryViewer tlm;
    if(FAILURE == tlm.init(vcm)) {
        logger.log_message("failed to initialize telemetry viewer");
        printf("failed to initialize telemetry viewer\n");
        exit(-1);
    }

    tlm.add_all();
    tlm.set_update_mode(TelemetryViewer::BLOCKING_UPDATE);

    measurement_info_t* m_info;
    std::string msg;
    std::string val;
    // uint32_t timestamp = 0;
    // unsigned char use_timestamp = 0;

    // main loop
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

        // construct the message
        msg = vcm->device;
        msg += " ";

        unsigned char first = 1;

        for(std::string meas : vcm->measurements) {
            m_info = vcm->get_info(meas);

            /**
            if(meas == "UPTIME") {
                if(SUCCESS == convert_uint(vcm, m_info, buff, &timestamp)) {
                    use_timestamp = 1;
                }
            }
            **/

            if(!first) {
                msg += ",";
            }
            first &= 0;


            if(SUCCESS == tlm.get_str(m_info, &val)) {
                msg += meas;
                msg += "=";
                msg += val;
            }
        }

        // we can add a timestamp in nano-seconds to the end of the line in Influx line protocol
        //if(use_timestamp) {
        //    uint64_t nanosec_time = timestamp;
        //    msg += std::to_string(nanosec_time * NANOSEC_PER_MILLISEC);
        //}

        // send the message
        ssize_t sent = -1;
        // std::cout << msg << "\n";
        sent = sendto(sockfd, msg.c_str(), msg.length(), 0,
            (struct sockaddr*)&servaddr, sizeof(servaddr));
        if(sent == -1) {
            logger.log_message("Failed to send UDP message");
            printf("Failed to send UDP message\n");
            // continue on
        }
    }
}
