#include <stdint.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "lib/vcm/vcm.h"
#include "lib/telemetry/TelemetryShm.h"

using namespace vcm;
using namespace std;
using namespace shm;

// shmctl should create shared memory before running this tool
// reads all measurements from shared memory

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

    mem.set_read_mode(TelemetryShm::BLOCKING_READ);

    // lock all packets
    if(mem.read_lock() != SUCCESS) {
        printf("failed to lock shared memory");
        return -1;
    }

    measurement_info_t* info;
    const uint8_t* buffer;
    for(std::string meas : vcm.measurements) {
        printf("%s: ", meas.c_str());

        info = vcm.get_info(meas);
        if(info == NULL) {
            printf("failed to get info for measurement %s, "
                    "likely a problem in VCM", meas.c_str());
            return -1;
        }

        for(location_info_t loc : info->locations) {
            buffer = mem.get_buffer(loc.packet_index);
            for(size_t i = 0; i < info->size; i++) {
                printf("%02x", buffer[i + loc.offset]);
            }
            printf("\t");
        }

        printf("\n");
    }

    // unlock all the packets
    if(mem.read_unlock() == FAILURE) {
        printf("failed to unlock shared memory");
        return -1;
    }
}
