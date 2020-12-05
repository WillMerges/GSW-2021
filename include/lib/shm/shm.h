/********************************************************************
*  Name: shm.h
*
*  Purpose: Header for utility functions for creating/modifying
*           shared memory.
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include <stdint.h>
#include "types.h"

// create shared memory block
RetType create_shm(size_t size);

// attach the current process to the shared memory block
RetType attach_to_shm();

// detach the current process to the shared memory block
RetType detach_from_shm();

// destroy the current shared memory
RetType destroy_shm();

// return pointer to shared memory block
uint8_t* get_shm_block();
