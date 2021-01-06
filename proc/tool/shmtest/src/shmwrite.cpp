#include "lib/shm/shm.h"
#include <stdint.h>
#include <stdio.h>

using namespace shm;

int main() {
    if(FAILURE == set_shmem_size(4)) {
        printf("Failed to set shared memory size\n");
        return -1;
    }

    if(FAILURE == attach_to_shm()) {
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
