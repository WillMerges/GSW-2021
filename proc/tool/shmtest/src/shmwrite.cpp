#include <stdint.h>
#include <stdio.h>
#include "lib/shm/shm.h"
#include "lib/vcm/vcm.h"

using namespace shm;
using namespace vcm;

// shmctl should NOT be turned on before this

int main() {
    // if(FAILURE == set_shmem_size(4)) {
    //     printf("Failed to set shared memory size\n");
    //     return -1;
    // }

    VCM vcm;
    vcm.packet_size = 4; // this is bad, only changing for testing

    if(FAILURE == create_shm(&vcm)) {
        printf("Failed to create shared memory\n");
        return -1;
    }

    if(FAILURE == attach_to_shm(&vcm)) {
        printf("Failed to attach to shared memory\n");
        return -1;
    }

    char b[4];
    b[0] = 'a';
    b[1] = 'b';
    b[2] = 'c';
    b[3] = 0;

    if(FAILURE == write_to_shm(b, 4)) {
        printf("Failed to write to shared memory\n");
        return -1;
    }

    if(FAILURE == detach_from_shm()) {
        printf("Failed to detach from shared memory\n");
        return -1;
    }
}
