/*
*  Interface to Linux serial device
*/
#include <stdint.h>
#include <stdlib.h>

// initialize a serial device named by the device file 'dev_file' (e.g. /dev/ttyUSB0)
// returns a serial descriptor for the device, or -1 on error
int serial_init(const char* dev_file);

// close an initialized serial device given by serial descriptor 'sd'
// returns 0 on success, -1 on error
int serial_close(int sd);

// returns number of bytes read/written, or -1 on error
// reads/writes to/from the serial device initialized at serial descriptor 'sd'
int serial_write(int sd, uint8_t* data, size_t len);
int serial_read(int sd, uint8_t* buff, size_t len);
