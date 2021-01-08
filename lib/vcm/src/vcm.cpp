#include "lib/vcm/vcm.h"
#include "lib/dls/dls.h"
#include "common/types.h"
#include <string>
#include <fstream>
#include <sstream>
#include <exception>
#include <unordered_map>

using namespace dls;
using namespace vcm;

VCM::VCM() {
    MsgLogger logger("VCM", "Constructor");

    // default values
    addr = port = -1;
    protocol = PROTOCOL_NOT_SET;
    packet_size = 0;

    // figure out default config file
    char* env = getenv("GSW_HOME");
    if(env == NULL) {
        logger.log_message("GSW_HOME environment variable not set!");
        throw new std::runtime_error("Environment error in VCM");
    }
    config_file = env;
    config_file += "/";
    config_file += DEFAULT_CONFIG_FILE;

    // init
    if(init() != SUCCESS) {
        throw new std::runtime_error("Error in VCM init");
    }
}

VCM::VCM(std::string config_file) {
    this->config_file = config_file;

    // default values
    addr = port = -1;
    protocol = PROTOCOL_NOT_SET;
    packet_size = 0;

    // init
    if(init() != SUCCESS) {
        throw new std::runtime_error("Error in VCM init");
    }
}

VCM::~VCM() {
    // free all pointers in addr_map
    for(auto i : addr_map) {
        delete i.second;
    }

    if(f->is_open()) {
        f->close();
    }
    delete f;
}

measurement_info_t* VCM::get_info(std::string measurement) {
    if(addr_map.count(measurement)) {
        return addr_map.at(measurement);
    } else {
        return NULL;
    }
}

RetType VCM::init() {
    MsgLogger logger;

    f = new std::ifstream(config_file.c_str());
    if(!f) {
        logger.log_message("Failed to open config file: "+config_file);
        return FAILURE;
    }

    // read the config file
    for(std::string line; std::getline(*f,line); ) {
        if(line == "" || !line.rfind("#",0)) { // blank or comment '#'
            continue;
        }

        // get 1st + 2nd tokens
        std::istringstream ss(line);
        std::string fst;
        ss >> fst;
        std::string snd;
        ss >> snd;

        // port or addr or protocol line
        if(snd == "=") {
            std::string third;
            ss >> third;
            if(fst == "addr") {
                try {
                    addr = std::stoi(third, NULL, 10);
                } catch(std::invalid_argument& ia) {
                    logger.log_message("Invalid addr in line: " + line);
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
            }
        } else if(fst != "" && snd != "") {
            measurement_info_t* entry = new measurement_info_t;
            entry->addr = (void*)packet_size;
            try {
                entry->size = (size_t)(std::stoi(snd, NULL, 10)); // this is in bits
            } catch(std::invalid_argument& ia) {
                logger.log_message("Invalid measurement size: " + line);
                return FAILURE;
            }
            packet_size += entry->size;
            //packet_size += (packet_size % 8); // add byte padding
            addr_map[fst] = entry;
            measurements.push_back(fst);
        } else {
            logger.log_message("Invalid line: " + line);
            return FAILURE;
        }
    }

    f->close();

    if(protocol == PROTOCOL_NOT_SET) {
        logger.log_message("Config file missing protocol: " + config_file);
        return FAILURE;
    } else if(protocol == UDP && (addr == -1 || port == -1)) {
        logger.log_message("Config file missing port or addr for UDP protocol: " + config_file);
        return FAILURE;
    }
    // } else if(packet_size % 8 != 0) {
    //     logger.log_message("Error byte aligning packet");
    //     return FAILURE;
    // }

    return SUCCESS;
}
