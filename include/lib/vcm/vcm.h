/********************************************************************
*  Name: vcm.h
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

    // int and float don't specify sizes, just what type they are
    // for example a single byte measurement may say int because it can be treated as one
    // TODO add doubles and other things?
    typedef enum {
        UNDEFINED_TYPE, INT_TYPE, FLOAT_TYPE, STRING_TYPE
    } measurement_type_t;

    typedef enum {
        SIGNED_TYPE, UNSIGNED_TYPE
    } measurement_sign_t;

    typedef enum {
        GSW_BIG_ENDIAN, GSW_LITTLE_ENDIAN
    } endianness_t;

    typedef enum {
        UDP, PROTOCOL_NOT_SET
    } protocol_t;

    typedef struct {
        void* addr; // offset into shmem
        size_t size; // bytes
        size_t l_padding; // bits of left padding (most significant bits)
        size_t r_padding; // bits of right padding (least significant bits)
        measurement_type_t type;
        measurement_sign_t sign;
    } measurement_info_t;

    class VCM {
    public:
        VCM(); // uses default config file
        VCM(std::string config_file);
        ~VCM();

        // returns NULL if no measurement with that name exists
        measurement_info_t* get_info(std::string measurement); // get the info of a measurement
        std::vector<std::string> measurements; // list of measurement names

        size_t packet_size; // bytes, size of packet after padding is added
        // size_t compressed_size; // bits, size of packet before padding added

        // TODO maybe put addr and port in another subclass
        int addr; // address and port (only for UDP right now)
        int port;
        protocol_t protocol;
        std::string config_file;
        std::string device;

        endianness_t recv_endianness; // endianness of the receiver
        endianness_t sys_endianness; // endianness of the system GSW is running on
    private:
        // local vars
        std::ifstream* f;
        std::unordered_map<std::string, measurement_info_t*> addr_map;

        // helper method(s)
        RetType init();
    };
}

#endif
