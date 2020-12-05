/********************************************************************
*  Name: ldms.h
*
*  Purpose: Header for vehicle configuration manager source.
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#ifndef VCM_H
#define VCM_H

#include <string>
#include <stdint.h>
#include <unordered_map>
#include "common/types.h"

#define DEFAULT_CONFIG_FILE "data/config"

// responsible for translating config file into addresses in shared mem
// address 0x00 is the first byte in shared mem
// currently gives addr and size of data in shared mem (after attaching itself in constructor)
// data needs to be sent into shm through this lib as well
namespace vcm {

    typedef struct {
        void* addr;
        size_t size;
    } measurement_info_t;

    typedef enum {
        UDP, PROTOCOL_NOT_SET
    } protocol_t;

    class VCM {
    public:
        VCM(); // uses default config file
        VCM(std::string config_file);
        ~VCM();
        measurement_info_t get_info(std::string measurement); // get the info of a measurement
        size_t packet_size;
        int addr; // UDP address and port
        int port;
        protocol_t protocol;
    private:
        // local vars
        std::string config_file;
        std::unordered_map<std::string, measurement_info_t*> addr_map;

        // helper method(s)
        RetType init();
    };
}

#endif
