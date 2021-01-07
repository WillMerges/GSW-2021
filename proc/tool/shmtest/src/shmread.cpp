#include <stdint.h>
#include <iostream>
#include <stdio.h>
#include "lib/vcm/vcm.h"
#include "lib/shm/shm.h"

using namespace vcm;
using namespace std;
using namespace shm;

// shmctl should NOT be turned on before this

int main() {
    // if(FAILURE == set_shmem_size(4)) {
    //     printf("Failed to set shared memory size\n");
    //     return -1;
    // }
    //
    // if(FAILURE == attach_to_shm()) {
    //     printf("Failed to attach to shared memory\n");
    //     return -1;
    // }
    VCM vcm;
    vcm.packet_size = 4; // this is bad, just changing for testing purposes
    if(FAILURE == attach_to_shm(&vcm)) {
        printf("Failed to attach to shared memory\n");
        return -1;
    }

    char b[4];

    if(FAILURE == read_from_shm(b, 4)) {
        printf("Failed to read from shared memory\n");
        return -1;
    }

    cout << b << '\n';


    if(FAILURE == destroy_shm()) {
        printf("Failed to destroy shared memory\n");
        return -1;
    }
}
