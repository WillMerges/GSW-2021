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

#define DEFAULT_CONFIG_DIR "data/default"

// TODO make endianess per measurement rather than per file

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

    typedef enum {
        ADDR_AUTO,   // automatically determine IP address
        ADDR_STATIC  // statically set IP address
        // TODO domain name lookup
    } addr_mode_t;

    // network device info
    typedef struct {
        uint32_t unique_id;
        addr_mode_t mode;
        void* addr_info; // depends on address mode
    } net_info_t;

    typedef struct {
        size_t size;
        uint32_t timeout; // time before packet is considered stale (in milliseconds) TODO implement this
        uint16_t port; // in host order (NOT network order)
        bool is_virtual;
    } packet_info_t;

    typedef struct {
        size_t offset; // offset into packet
        uint32_t packet_index; // which packet
    } location_info_t;

    typedef struct {
        std::vector<location_info_t> locations; // locations of this measurement
        size_t size;                            // size in bytes
        endianness_t endianness;
        measurement_type_t type;
        measurement_sign_t sign;
    } measurement_info_t;

    class VCM {
    public:
        VCM(); // uses default config file
        VCM(std::string config_file);
        ~VCM();

        // initialize
        RetType init();

        // returns NULL if no measurement with that name exists
        measurement_info_t* get_info(std::string& measurement); // get the info of a measurement
        std::vector<std::string> measurements; // list of measurement names

        std::vector<packet_info_t*> packets; // list of packets

        // get network device info
        // returns NULL on error
        net_info_t* get_net(std::string& device_name);
        std::vector<std::string> net_devices; // list of network device names

        // TODO maybe put addr and port in another subclass
        // TODO maybe get rid of addr altogether, it isn't currently used anywhere
        // uint32_t addr; // address and port (only for UDP right now) ACTUALLY each packet specifies a port but the address of the ground computer doesn't change
        uint16_t port; // port that receiver sends from and ground station should send to
        uint32_t multicast_addr;
        protocol_t protocol;
        std::string config_dir;
        std::string config_file;
        std::string trigger_file;
        std::string device;

        endianness_t sys_endianness; // endianness of the system GSW is running on

        uint32_t num_packets; // number of telemetry packets
    private:
        // local vars
        std::ifstream* f;
        std::unordered_map<std::string, measurement_info_t*> addr_map;
        std::unordered_map<std::string, net_info_t*> net_map;
    };
}

#endif
