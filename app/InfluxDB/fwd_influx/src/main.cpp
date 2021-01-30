// forwards packets from shared mem. to InfluxDB using UDP line protocol

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
#include "lib/shm/shm.h"
#include "lib/dls/dls.h"
#include "common/types.h"

// view telemetry memory live
// run as mem_view [-f path_to_config_file]

// TODO
// remove printfs? or add verbose mode

using namespace vcm;
using namespace shm;
using namespace dls;

#define INFLUXDB_UDP_PORT 8089
#define INFLUXDB_ADDR "127.0.0.1"

int sockfd;
unsigned char sock_open = 0;

void sighandler(int signum) {
    if(sock_open) {
        close(sockfd);
    }

    exit(signum);
}

int main(int argc, char* argv[]) {
    MsgLogger logger("db fwd");

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
    try {
        if(config_file == "") {
            vcm = new VCM(); // use default config file
        } else {
            vcm = new VCM(config_file); // use specified config file
        }
    } catch (const std::runtime_error& e) {
        std::cout << e.what() << '\n';
        return FAILURE;
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

    // attach to shmem
    if(FAILURE == attach_to_shm(vcm)) {
        logger.log_message("unable to attach mem_view process to shared memory");
        printf("unable to attach mem_view process to shared memory\n");
        return FAILURE;
    }

    unsigned char* buff = new unsigned char[vcm->packet_size];
    memset((void*)buff, 0, vcm->packet_size); // zero the buffer

    measurement_info_t* m_info;
    size_t addr = 0;

    std::string msg;

    // main loop
    while(1) {
        // read from shared memoery
        if(FAILURE == read_from_shm_block((void*)buff, vcm->packet_size)) {
            logger.log_message("failed to read from shared memory");
            // ignore and continue
        }

        // construct the message
        msg = vcm->device;
        msg += " ";

        unsigned char first = 1;

        for(std::string meas : vcm->measurements) {
            if(!first) {
                msg += ",";
            }
            first &= 0;

            m_info = vcm->get_info(meas);
            addr = (size_t)m_info->addr;

            // add data
            if(m_info->type == INT_TYPE) {
                unsigned char val[sizeof(int)];
                memset(val, 0, sizeof(int));
                for(size_t i = 0; i < m_info->size; i++) {
                    val[i] = buff[addr + i]; // assumes little endian
                }

                msg += meas;
                msg += "=";
                msg += std::to_string(*((int*)(val)));
            } else if(m_info->type == FLOAT_TYPE) {
                unsigned char val[sizeof(float)];
                memset(val, 0, sizeof(float));
                for(size_t i = 0; i < m_info->size; i++) {
                    val[i] = buff[addr + i]; // assumes little endian
                }

                msg += meas;
                msg += "=";
                msg += std::to_string(*((float*)(val)));
            } else {
                // don't forward
            }
        }

        // send the message
        ssize_t sent = -1;
        sent = sendto(sockfd, msg.c_str(), msg.length(), 0,
            (struct sockaddr*)&servaddr, sizeof(servaddr));
        if(sent == -1) {
            logger.log_message("Failed to send UDP message");
            printf("Failed to send UDP message\n");
            // continue on
        }
    }
}
