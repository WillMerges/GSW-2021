#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "lib/shm/shm.h"
#include "lib/vcm/vcm.h"
#include "lib/telemetry/TelemetryShm.h"

using namespace shm;
using namespace vcm;

// shmctl should create shared memory before running this tool
// set's all bytes in shared memory to 0xCF

int main() {
    VCM vcm; //default vcm

    if(vcm.init() == FAILURE) {
        printf("failed to initialize VCM");
        return -1;
    }

    TelemetryShm mem;

    if(mem.init(&vcm) == FAILURE) {
        printf("failed to init telemetry shm");
        return -1;
    }

    if(mem.open() == FAILURE) {
        printf("failed to attach to shared memory");
        return -1;
    }

    packet_info_t* info;
    uint8_t* buffer;
    for(uint32_t i = 0; i < vcm.num_packets; i++) {
        info = vcm.packets[i];

        if(info->is_virtual) {
            // skip the virtual telemetry packet
            continue;
        }

        buffer = (uint8_t*)malloc(sizeof(info->size));
        memset(buffer, 0xCF, info->size);
        mem.write(i, buffer);
        free(buffer);
    }
}
