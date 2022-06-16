/*******************************************************************************
* Name: main.cpp
*
* Purpose: DARWIN controller
*          Executes remote AT commands on XBee on DARWIN
*
* Author: Will Merges
*
* Usage ./darwin [path_to_serial_device] [option]
*
* Options include:
*   init        - initialize the onboard XBee, placing it into API mode
*   net_id [id] - set the network id of the XBee, 'id' is 16-bits in hexadecimal
*   vtx-on      - remotely toggle the digial I/O that turns on the video transmitter
*   vtx-off     - remotely toggle the digial I/O that turns on the video transmitter
*
*
* RIT Launch Initiative
*******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <unistd.h>

#include "lib/dls/dls.h"
#include "lib/dev/serial/serial.h"
#include "lib/dev/xbee/xbee.h"

using namespace dls;

// serial descriptor
int sd;

// wraps serial write
int xbee_write(uint8_t* buff, size_t len) {
    return serial_write(sd, buff, len);
}

// wraps usleep
void xbee_delay(uint32_t ms) {
    usleep(ms * 1000);
}

int main(int argc, char* argv[]) {
    MsgLogger logger("DARWIN");

    if(argc < 3) {
        printf("usage: ./darwin [path_to_serial_device] [option]\n");
        logger.log_message("invalid arguments");
        return -1;
    }

    sd = serial_init(argv[1]);
    if(sd == -1) {
        printf("error initializing serial device\n");
        logger.log_message("error initializing serial device");
        return -1;
    }

    // set write handler
    xb_set_handler(&xbee_write);

    // parse passed in command
    std::string arg = argv[2];
    if("init" == arg) {
        if(XB_OK != xb_init(&xbee_write, &xbee_delay)) {
            printf("init failed\n");
            logger.log_message("init failed");
            return -1;
        }
    } else if("net_id" == arg) {
        if(argc < 4) {
            printf("missing argument for 'net_id'\n");
            logger.log_message("missing argument for 'net_id'");
            return -1;
        }

        char* endptr;
        uint16_t id = (uint16_t)strtol(argv[3], &endptr, 16);

        if(endptr != NULL) {
            printf("invalid argument for 'net_id'\n");
            logger.log_message("invalud argument for 'net_id'");
            return -1;
        }

        if(XB_OK != xb_set_net_id(id)) {
            printf("set net id failed\n");
            logger.log_message("set net id failed");
            return -1;
        }
    } else if ("vtx-on" == arg) {
        if(XB_OK != xb_cmd_remote_dio(XB_DIO12, XB_DIO_HIGH)) {
            printf("failed to command remote digital I/O high\n");
            logger.log_message("failed to command remote digital I/O hig");
            return -1;
        }
    } else if ("vtx-off" == arg) {
        if(XB_OK != xb_cmd_remote_dio(XB_DIO12, XB_DIO_LOW)) {
            printf("failed to command remote digital I/O low\n");
            logger.log_message("failed to command remote digital I/O low");
            return -1;
        }
    } else {
        printf("unsupported option\n");
        logger.log_message("unsupported option");
        return -1;
    }

    serial_close(sd);
    return 0;
}
