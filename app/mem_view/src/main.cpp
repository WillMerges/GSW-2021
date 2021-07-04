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
#include "common/types.h"

// view telemetry memory live
// run as mem_view [-f path_to_config_file]

using namespace vcm;
using namespace shm;
using namespace dls;

int main(int argc, char* argv[]) {
    MsgLogger logger("mem_view");

    logger.log_message("starting mem_view");

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

    int count = 0; // number of measurements

    unsigned char* buff = new unsigned char[vcm->packet_size];
    memset((void*)buff, 0, vcm->packet_size); // zero the buffer

    unsigned int max_length = 0;
    for(std::string it : vcm->measurements) {
        count++;
        if(it.length() > max_length) {
            max_length = it.length();
        }
    }

    // clear the screen
    printf("\033[2J");

    measurement_info_t* m_info;
    size_t addr = 0;
    while(1) {
        for(std::string meas : vcm->measurements) {
            m_info = vcm->get_info(meas);
            addr = (size_t)m_info->addr;

            printf("%s  ", meas.c_str());

            // print extra spaces
            for(size_t i = 0; i < max_length - meas.length(); i++) {
                printf(" ");
            }

            for(size_t i = 0; i < m_info->size; i++) {
                printf("0x%02X ", buff[addr + i]);
            }
            printf("\n");
        }


        // read from shared memory
        if(FAILURE == read_from_shm_block((void*)buff, vcm->packet_size)) {
            logger.log_message("failed to read from shared memory");
            printf("failed to read from shared memory\n");
            // ignore and continue
        } else {
            // clear the screen
            printf("\033[2J");
        }

        usleep(1000);
    }
}
