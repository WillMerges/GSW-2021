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
#include <fstream>
#include <unordered_map>
#include <vector>
#include "common/types.h"

#define DEFAULT_CONFIG_FILE "data/config"

// responsible for translating config file into addresses in shared mem
// address 0x00 is the first byte in shared mem
// currently gives addr and size of data in shared mem (after attaching itself in constructor)
// data needs to be sent into shm through this lib as well
namespace vcm {

    typedef struct {
        void* addr; // offset into shmem
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
        measurement_info_t* get_info(std::string measurement); // get the info of a measurement
        std::vector<std::string> measurements; // list of measurement names
        size_t packet_size;
        // TODO maybe put addr and port in another subclass
        int src; // address and port (only for UDP right now)
        int port;
        protocol_t protocol;
        std::string config_file;
    private:
        // local vars
        std::ifstream* f;
        std::unordered_map<std::string, measurement_info_t*> addr_map;

        // helper method(s)
        RetType init();
    };
}

#endif
