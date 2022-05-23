/*******************************************************************************
* Name: main.cpp
*
* Purpose: XBee bridge
*          Receives XBee frames over serial and sends them over raw IPv4
*          Assumes all frames are UDP (sets protocol flag to UDP)
*
*          Destination IP of packets is either this machine or the multicast
*          address if one is set in the vehicle configuration
*
*          Source IP is this device
*
* Author: Will Merges
*
* Usage ./xbee_bridge [path_to_serial_device] [-f optional_path_to_VCM_file]
*       The -f flag is optional
*
* RIT Launch Initiative
*******************************************************************************/
#include "lib/dls/dls.h"
#include "lib/vcm/vcm.h"
#include "lib/dev/serial/serial.h"
#include "lib/dev/xbee/xbee.h"
#include <netinet/ip.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace dls;
using namespace vcm;


int sd = -1;
struct sockaddr_in dst_addr;

int serial_sd = 1;

void packet_received(uint8_t* buff, size_t len, uint64_t addr) {
    printf("packet received from address: %lu\n", addr);
    sendto(sd, (void*)buff, len, 0, (struct sockaddr*)&dst_addr, sizeof(dst_addr));
}

int main(int argc, char* argv[]) {
    MsgLogger logger("xbee_bridge");

    if(argc < 2) {
        printf("usage: ./xbee_bridge [path_to_serial_device] [-f optional_path_to_VCM_file]\n");
        logger.log_message("invalid number of args");
        exit(-1);
    }

    std::string dev = argv[1];
    logger.log_message("starting XBee network bridge on device: " + dev);
    printf("starting XBee network bridge on device: %s\n", dev.c_str());

    serial_sd = serial_init(argv[1]);
    if(serial_sd == -1) {
        printf("failed to initialize serial\n");
        logger.log_message("failed to initialize serial");

        exit(-1);
    }

    std::string config_file = "";

    for(int i = 2; i < argc; i++) {
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

    VCM* veh;
    if(config_file == "") {
        veh = new VCM(); // use default config file
    } else {
        veh = new VCM(config_file); // use specified config file
    }

    if(FAILURE == veh->init()) {
        logger.log_message("failed to initialize VCM");
        printf("failed to initialize VCM\n");

        exit(-1);
    }

    sd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);

    if(sd == -1) {
        printf("failed to create socket, permissions error?\n");
        logger.log_message("failed to create socket");

        exit(-1);
    }

    struct sockaddr_in src_addr;
    src_addr.sin_family = AF_INET;
    src_addr.sin_port = 0;
    src_addr.sin_addr.s_addr = INADDR_ANY;

    if(-1 == bind(sd, (struct sockaddr*)&src_addr, sizeof(src_addr))) {
        printf("socket bind failed, permissions error?\n");
        logger.log_message("socket bind failed");

        exit(-1);
    }

    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = 0;

    if(veh->multicast_addr) {
        dst_addr.sin_addr.s_addr = veh->multicast_addr;
        std::string ascii_addr = inet_ntoa(dst_addr.sin_addr);
        logger.log_message("bridging to multicast address: " + ascii_addr);
        printf("bridging to multicast addr: %s\n", ascii_addr.c_str());
    } else {
        inet_aton("127.0.0.1", &(dst_addr.sin_addr));
        logger.log_message("bridging to localhost");
        printf("bridging to localhost\n");
    }

    xb_attach_rx_callback(&packet_received);

    logger.log_message("bridge initialized");
    printf("bridge initialized\n");

    xb_rx_request req;
    req.len = 0;
    int read;
    while(1) {
        xb_rx_complete(&req);

        read = serial_read(serial_sd, req.buff, req.len);
        if(read == -1) {
            req.len = 0;
        } else {
            req.len = (size_t)read;
        }
    }
}
