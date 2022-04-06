#include "lib/vcm/vcm.h"
#include "lib/dls/dls.h"
#include "common/types.h"
#include <string>
#include <fstream>
#include <sstream>
#include <exception>
#include <unordered_map>
#include <unordered_set>
#include <endian.h>
#include <arpa/inet.h>

using namespace dls;
using namespace vcm;

VCM::VCM() {
    MsgLogger logger("VCM", "Constructor");

    // default values
    port = 0; // treat zero as an invalid port
    protocol = PROTOCOL_NOT_SET;
    multicast_addr = 0; // treat address of zero as invalid
    // packet_size = 0;
    num_packets = 0;
    device = "";
    trigger_file = "";

    if(__BYTE_ORDER == __BIG_ENDIAN) {
        sys_endianness = GSW_BIG_ENDIAN;
    } else if(__BYTE_ORDER == __LITTLE_ENDIAN) {
        sys_endianness = GSW_LITTLE_ENDIAN;
    } else {
        logger.log_message("Could not determine endianness of system, assuming little endian");
        sys_endianness = GSW_LITTLE_ENDIAN;
    }

    // figure out default config file
    char* env = getenv("GSW_HOME");
    if(env == NULL) {
        logger.log_message("GSW_HOME environment variable not set!");
        throw new std::runtime_error("Environment error in VCM");
    }
    config_dir = env;
    config_dir += "/";
    config_dir += DEFAULT_CONFIG_DIR;
    config_file = config_dir + "/config";

    // init
    // NEW CHANGE: user has to call init themself
    // if(init() != SUCCESS) {
    //     throw new std::runtime_error("Error in VCM init");
    // }
}

VCM::VCM(std::string config_file) {
    this->config_file = config_file;

    // default values
    protocol = PROTOCOL_NOT_SET;
    // packet_size = 0;
    num_packets = 0;
    // compressed_size = 0;
    device = "";

    // init
    // NEW CHANGE: user has to call init themself
    // if(init() != SUCCESS) {
    //     throw new std::runtime_error("Error in VCM init");
    // }
}

VCM::~VCM() {
    // free all pointers in addr_map
    for(auto i : addr_map) {
        delete i.second;
    }

    // free all pointers in packets
    for(auto i : packets) {
        delete i;
    }

    if(f) {
        if(f->is_open()) {
            f->close();
        }

        delete f;
    }
}

measurement_info_t* VCM::get_info(std::string& measurement) {
    if(addr_map.count(measurement)) {
        return addr_map.at(measurement);
    } else {
        return NULL;
    }
}

RetType VCM::init() {
    MsgLogger logger("VCM", "init");

    f = new std::ifstream(config_file.c_str());
    if(!f) {
        logger.log_message("Failed to open config file: "+config_file);
        return FAILURE;
    }

    // hash set to check uniqueness of telemetry packet ports
    std::unordered_set<uint16_t> port_set;

    // read the config file
    for(std::string line; std::getline(*f,line); ) {
        if(line == "" || !line.rfind("#",0)) { // blank or comment '#'
            continue;
        }

        // get 1st + 2nd tokens
        // this is really ugly ¯\_(ツ)_/¯
        std::istringstream ss(line);
        std::string fst;
        ss >> fst;
        std::string snd;
        ss >> snd;
        std::string third;
        ss >> third;

        // port or addr or protocol line
        if(snd == "=") {
            if(fst == "multicast") {
                try {
                    // IPv4 is currently only type supported
                    inet_pton(AF_INET, third.c_str(), &multicast_addr);
                } catch(std::invalid_argument& ia) {
                    logger.log_message("Invalid multicast address in line: " + line);
                    return FAILURE;
                }
            } else if(fst == "port") {
                try {
                    port = std::stoi(third, NULL, 10);
                } catch(std::invalid_argument& ia) {
                    logger.log_message("Invalid port in line: " + line);
                    return FAILURE;
                }
            } else if(fst == "protocol") {
                if(third == "udp") {
                    protocol = UDP;
                } else {
                    logger.log_message("Unrecogonized protocol on line: " + line);
                    return FAILURE;
                }
            } else if(fst == "name") {
                device = third;
            } else if(fst == "triggers") {
                trigger_file = config_dir + "/" + third;
            } else {
                logger.log_message("Invalid line: " + line);
                return FAILURE;
            }
        } else if(snd == "{") { // start of a telemetry packet
            packet_info_t* packet = new packet_info_t;
            packet->size = 0;

            if(fst == "virtual") {
                packet->port = 0;
                packet->is_virtual = true;
            } else {
                packet->is_virtual = false;

                // get the network port for this packet
                // this is the port the packet is sent TO
                try {
                    packet->port = std::stoi(fst, NULL, 10);
                } catch(std::invalid_argument& ia) {
                    logger.log_message("Invalid port in line: " + line);
                    return FAILURE;
                }

                if(port_set.find(packet->port) != port_set.end()) {
                    logger.log_message("Telemetry packets must have unique port numbers");
                    return FAILURE;
                }

                port_set.insert(packet->port);
            }

            bool done = false;
            // we don't allow comments or empty lines after starting a packet
            // BUT we don't check for anything following a measurement name, we only look at the first token
            for(std::string line; std::getline(*f,line); ) {
                std::istringstream ss(line);
                std::string token;
                ss >> token;

                if(token == "}") { // end of packet
                    done = true;
                    break;
                } else { // token is a measurement!
                    measurement_info_t* meas;
                    if(addr_map.count(token)) {
                        meas = addr_map.at(token);
                    } else {
                        logger.log_message("Measurement " + token + " does not exist");
                        return FAILURE;
                    }

                    location_info_t loc;
                    loc.offset = packet->size;
                    loc.packet_index = num_packets;
                    meas->locations.push_back(loc); // copy struct in, less memory to track and it's small
                    packet->size += meas->size;
                }
            }

            if(done) {
                packets.push_back(packet);
                num_packets++;
            } else {
                logger.log_message("Reached end of file before end of packet");
                return FAILURE;
            }
        } else { // measurement definition
            std::string fourth;
            ss >> fourth;
            std::string fifth;
            ss >> fifth;

            if(fst == "" || snd == "") {
                logger.log_message("Missing information: " + line);
                return FAILURE;
            }

            measurement_info_t* entry = new measurement_info_t;

            try {
                entry->size = (size_t)(std::stoi(snd, NULL, 10));
            } catch(std::invalid_argument& ia) {
                logger.log_message("Invalid measurement size: " + line);
                return FAILURE;
            }

            // check for type (optional, default is undefined)
            if(third == "int") {
                entry->type = INT_TYPE;
            } else if(third == "float") {
                entry->type = FLOAT_TYPE;
            } else if(third == "string") {
                entry->type = STRING_TYPE;
            } else if(third == "") {
                entry->type = UNDEFINED_TYPE;
            } else {
                logger.log_message("Invalid type specified: " + third);
                return FAILURE;
            }

            std::string tok = fourth;

            uint8_t endianness_set = 0;

            if(tok == "big") {
                entry->endianness = GSW_BIG_ENDIAN;
                endianness_set = 1;
            } else if(tok == "little") {
                entry->endianness = GSW_LITTLE_ENDIAN;
                endianness_set = 1;
            }

            tok = fifth;
            if(!endianness_set) {
                // repeated code, this is bad lol
                if(tok == "big") {
                    entry->endianness = GSW_BIG_ENDIAN;
                    endianness_set = 1;
                } else if(tok == "little") {
                    entry->endianness = GSW_LITTLE_ENDIAN;
                    endianness_set = 1;
                } else if(tok == "") {
                    // default is the system endianness
                    entry->endianness = sys_endianness;
                } else {
                    logger.log_message("invalid token: " + tok);
                    return FAILURE;
                }

                tok = fourth;
            }

            if(tok == "unsigned") {
                entry->sign = UNSIGNED_TYPE;
            } else if(tok == "signed") {
                entry->sign = SIGNED_TYPE;
            } else if(tok == "") {
                // default is signed
                entry->sign = SIGNED_TYPE;
            } else {
                logger.log_message("Invalid token: " + tok);
                return FAILURE;
            }

            addr_map[fst] = entry;
            measurements.push_back(fst);
        }
    }

    f->close();

    // check for unset mandatory configuration items
    if(protocol == PROTOCOL_NOT_SET) {
        logger.log_message("Config file missing protocol: " + config_file);
        return FAILURE;
    // } else if(protocol == UDP && (addr == -1 || port == -1)) {
    } else if(protocol == UDP && port == 0) {
        // logger.log_message("Config file missing port or addr for UDP protocol: " + config_file);
        logger.log_message("Config file missing port for UDP protocol: " + config_file);
        return FAILURE;
    }

    return SUCCESS;
}
