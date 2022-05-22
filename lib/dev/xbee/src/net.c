/*
*   Generic network functions
*   ntoh and hton functions assume system is little endian order (true for ARM)
*
*   TODO this was originally from the NIGEL repo, we can just use the 
*   built-in functions for 16 and 32 bit conversions
*/
#include "lib/dev/xbee/net.h"
#include <stdlib.h>

// little endian to big endian

uint16_t hton16(uint16_t n) {
    uint8_t ret[sizeof(uint16_t)];

    for(ssize_t i = sizeof(uint16_t) - 1; i >= 0; i--) {
        ret[i] = n;
        n >>= 8;
    }

    return *((uint16_t*)ret);
}

uint32_t hton32(uint32_t n) {
    uint8_t ret[sizeof(uint32_t)];

    for(ssize_t i = sizeof(uint32_t) - 1; i >= 0; i--) {
        ret[i] = n;
        n >>= 8;
    }

    return *((uint32_t*)ret);
}

uint64_t hton64(uint64_t n) {
    uint8_t ret[sizeof(uint64_t)];

    for(ssize_t i = sizeof(uint64_t) - 1; i >= 0; i--) {
        ret[i] = n;
        n >>= 8;
    }

    return *((uint64_t*)ret);
}

// big endian to little endian

uint16_t ntoh16(uint16_t n) {
    return hton16(n);
}

uint32_t ntoh32(uint32_t n) {
    return hton32(n);
}

uint64_t ntoh64(uint64_t n) {
    return hton64(n);
}
