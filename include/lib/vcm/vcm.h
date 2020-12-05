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


// responsible for setting up telemetry shared mem and providing accessor functions to telemetry data
// currently gives addr and size of data in shared mem (after attaching itself in constructor)
// data needs to be sent into shm through this lib as well
namespace vcm {

    typedef struct {
        void* addr;
        size_t size;
    } measurement_info_t;

    class VCM {
    public:
        LDMS(); // uses default config file
        LDMS(std::string config_file);
        measurement_info_t get_info(std::string measurement); // get the info of a measurement
    private:
        // local vars
        std::string config_file;
        std::unordered_map<std::string, void*> addr_map;

        // helper methods
        RetType parse_config_file();
        RetType setup_mem_map();
    };
}

#endif
