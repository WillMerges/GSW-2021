/*
*   Generic network helper functions
*
*   Will Merges @ RIT Launch Initiative
*/
#ifndef NET_H
#define NET_H

#include <stdint.h>

// hardware to network order conversions
uint16_t hton16(uint16_t n);
uint32_t hton32(uint32_t n);
uint64_t hton64(uint64_t n);

// network to hardware order conversions
uint16_t ntoh16(uint16_t n);
uint32_t ntoh32(uint32_t n);
uint64_t ntoh64(uint64_t n);

#endif
