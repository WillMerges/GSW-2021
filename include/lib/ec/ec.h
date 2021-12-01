/*
* Engine Controller Library Header
*/
#ifndef EC_H
#define EC_H

#include <stdint.h>

typedef struct {
    uint32_t seq_num; // sequence number
    uint16_t control; // control
    uint16_t state;   // state
} ec_command_t;

#endif
