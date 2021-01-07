#include <stdio.h>
#include <string.h>
#include <iostream>
#include "lib/vcm/vcm.h"
#include "lib/shm/shm.h"
#include "common/types.h"
#include "lib/dls/dls.h"

// run as shmctl -on or shmctl -off to create and destroy shared memory for the
// current VCM file

using namespace vcm;
using namespace shm;
using namespace dls;

bool on = false;
bool off = false;

int main(int argc, char* argv[]) {
    MsgLogger logger("SHMCTL", "");

    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "-on") && !off) {
            on = true;
        } else if(!strcmp(argv[i], "-off") && !on) {
            off = true;
        } else {
            printf("Invalid argument: %s\n", argv[i]);
            return -1;
        }
    }

    VCM* vcm;
    try {
        vcm = new VCM(); // use default config file
    } catch (const std::runtime_error& e) {
        std::cout << e.what() << '\n';
        exit(-1);
    }

    if(on) {
        printf("creating shared memory\n");
        logger.log_message("creating shared memory");
        if(FAILURE == create_shm(vcm)) {
            printf("Failed to create shared memory\n");
            logger.log_message("Failed to create shared memory");
            return FAILURE;
        }
        return SUCCESS;
    } else if(off) {
        if(FAILURE == attach_to_shm(vcm)) {
            printf("Shared memory not created, nothing to destroy\n");
            logger.log_message("Shared memory not created, nothing to destroy");
            return FAILURE;
        }
        printf("destroying shared memory\n");
        logger.log_message("destroying shared memory");
        if(FAILURE == destroy_shm()) {
            printf("Failed to destroy shared memory\n");
            logger.log_message("Failed to destroy shared memory");
            return FAILURE;
        }
        return SUCCESS;
    }
}
