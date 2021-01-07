#include <stdint.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "lib/vcm/vcm.h"
#include "lib/shm/shm.h"

using namespace vcm;
using namespace std;
using namespace shm;

// shmctl should NOT be turned on before this

int main() {
    VCM vcm;
    //vcm.packet_size = 4; // this is bad, just changing for testing purposes

    if(FAILURE == attach_to_shm(&vcm)) {
        printf("Failed to attach to shared memory\n");
        return -1;
    }

    char b[4];

    while(1) {
        // NOTE: 3 different ways to read, using the blocking method currently (hardest to test)
        if(SUCCESS == read_from_shm_block(b, 4)) {
        // if(SUCCESS == read_from_shm_if_updated(b, 4)) {
        // if(SUCCESS == read_from_shm(b,4)) {
            cout << b << '\n';
        }
    }

    if(FAILURE == detach_from_shm()) {
         printf("Failed to detach from shared memory\n");
         return -1;
    }
}
