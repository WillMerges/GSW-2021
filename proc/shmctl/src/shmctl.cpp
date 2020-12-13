#include <stdio.h>
#include <string.h>
#include "lib/vcm/vcm.h"
#include "lib/shm/shm.h"
#include "common/types.h"
#include "lib/dls/dls.h"

// run as shmctl -on or shmctl -off to create and destroy shared memory for the
// current VCM file

using namespace vcm;
using namespace shm;

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

    VCM vcm;
    set_shmem_size(vcm.packet_size);

    if(on) {
        printf("creating shared memory\n");
        logger.log_message("destroying shared memory");
        return attach_to_shm();
    } else if(off) {
        printf("destroying shared memory\n");
        logger.log_message("destroying shared memory");
        return destroy_shm();
    }
}
