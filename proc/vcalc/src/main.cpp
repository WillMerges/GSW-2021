/********************************************************************
*  Name: main.cpp
*
*  Purpose: Virtual telemetry calculation process, waits for updates
*           to telemetry and calculates virtual telemetry measurements.
*
*  Usage:
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/telemetry/TelemetryShm.h"
#include "lib/calc/calc.h"
#include "lib/dls/dls.h"
#include <unordered_set>
#include <signal.h>
#include <string.h>

using namespace dls;
using namespace vcm;
using namespace calc;

VCM* veh;
TelemetryViewer tlm;
TelemetryShm tshm;

void sighandler(int signum) {
    tshm.sighandler();
    exit(signum);
}

int main(int argc, char** argv) {
    MsgLogger logger("VCALC", "main");
    logger.log_message("starting vcalc process");

    // interpret the 1st argument as a config_file location if available
    std::string config_file = "";
    if(argc > 1) {
        config_file = argv[1];
    }

    if(config_file == "") {
        veh = new VCM(); // use default config file
    } else {
        veh = new VCM(config_file); // use specified config file
    }

    // init VCM
    if(veh->init() != SUCCESS) {
        logger.log_message("failed to initialize VCM");
        return -1;
    }

    if(veh->vcalc_file == "") {
        logger.log_message("no vcalc file specified");
        return 1;
    }

    // setup telemetry shm
    if(tshm.init(veh) != SUCCESS) {
        logger.log_message("failed to init telemetry shm");
        return -1;
    }

    if(tshm.open() != SUCCESS) {
        logger.log_message("failed to open telemetry shm");
        return -1;
    }

    // setup telemetry viewer
    if(tlm.init(veh, &tshm)) {
        logger.log_message("failed to init telemetry viewer");
        return -1;
    }

    // add signal handlers
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGFPE, sighandler);
    signal(SIGABRT, sighandler);

    // parse the virtual file
    std::vector<vcalc_t> entries;
    if(parse_vfile(veh, &entries) != SUCCESS) {
        logger.log_message("failed to parse vcalc file");
        return FAILURE;
    }

    logger.log_message("successfully parsed vcalc file");

    // indexes are packet id's, each index contains a list of triggers
    // lists contain measurements that need to be calculated when the packet is
    std::vector<vcalc_t>* triggers = new std::vector<vcalc_t>[veh->num_packets];

    // tracks id's for each packet to make sure each trigger is only included once
    // e.g. if a conversion needs two measurements that are in the same packet
    std::unordered_set<size_t>* ids = new std::unordered_set<size_t>[veh->num_packets];

    // track which packets we need to check for updates too
    bool* used_packets = new bool[veh->num_packets];

    // output buffers
    uint8_t** packet_buffers = new uint8_t*[veh->num_packets];

    // loggers to write packets out
    std::vector<PacketLogger*> loggers;

    // sizes of each packet
    std::vector<size_t> packet_sizes;

    // set defaults
    for(uint32_t i = 0; i < veh->num_packets; i++) {
        used_packets[i] = false;
        packet_buffers[i] = NULL;

        PacketLogger* logger = new PacketLogger(veh->device + "(" + std::to_string(i) + ")");
        loggers.push_back(logger);

        packet_sizes.push_back(veh->packets[i]->size);
    }

    // add events for each packet
    for(vcalc_t v : entries) {
        // allocate buffers for each output location
        for(location_info_t loc : v.out->locations) {
            // the measurement may exists in a virtual packet and in regular telemetry
            // we only want to allocate buffers for virtual packets
            if(veh->packets[loc.packet_index]->is_virtual) {
                if(packet_buffers[loc.packet_index] == NULL) {
                    packet_buffers[loc.packet_index] = new uint8_t[packet_sizes[loc.packet_index]];
                    memset(packet_buffers[loc.packet_index], 0, packet_sizes[loc.packet_index]);
                } // otherwise we've already allocated this buffer
            }
        }

        // iterate through each argument
        for(measurement_info_t* arg : v.args) {
            for(location_info_t loc : arg->locations) {
                if(ids[loc.packet_index].find(v.unique_id) == ids[loc.packet_index].end()) {
                    // add this as a trigger
                    triggers[loc.packet_index].push_back(v);

                    // mark this packet to check for updates
                    used_packets[loc.packet_index] = true;

                    // make sure we don't add this trigger for the same packet again
                    // e.g. if both arguments are from the same packet, only need one trigger
                    ids[loc.packet_index].insert(v.unique_id);
                }
            }
        }
    }

    delete[] ids;

    // track which packets to check for updates
    std::vector<uint32_t> arg_packets;

    // add packets we need to telemetry viewer
    // also allocate buffers for output packet(s)
    for(uint32_t i = 0; i < veh->num_packets; i++) {
        if(used_packets[i]) {
            tlm.add(i);
            arg_packets.push_back(i);

            // TODO allocate loggers here, not for every single packet type
        }
    }

    delete[] used_packets;

    // set update mode to blocking
    tlm.set_update_mode(TelemetryViewer::BLOCKING_UPDATE);

    // loop
    std::vector<arg_t> arg_list;
    arg_t arg;
    RetType ret;
    std::unordered_set<uint32_t> triggers_processed;
    std::unordered_set<uint32_t> output_packets;
    while(1) {
        if(tlm.update() != SUCCESS) {
            logger.log_message("failed to update telemetry");
            continue;
        }

        output_packets.clear();

        for(uint32_t packet_id : arg_packets) {
            if(tshm.updated[packet_id]) {
                // this packet updated, we need to calculate values from it

                for(vcalc_t calculation : triggers[packet_id]) {
                    if(triggers_processed.find(calculation.unique_id) == triggers_processed.end()) {
                        // this is a new trigger

                        // generate the argument list
                        arg_list.clear();
                        for(measurement_info_t* meas : calculation.args) {
                            arg.meas = meas;
                            if(tlm.latest_data(meas, &arg.addr) != SUCCESS) {
                                logger.log_message("failed to retrieve latest data for measurement");
                                continue;
                            }

                            arg_list.push_back(arg);
                        }

                        // run the calculation for each virtual output location
                        for(location_info_t loc : calculation.out->locations) {
                            if(!veh->packets[loc.packet_index]->is_virtual) {
                                // regular telemetry, don't write anything
                                continue;
                            }

                            // TODO make this better? e.g. calculate once then write multiple times?
                            ret = (*(calculation.convert_function))(calculation.out, packet_buffers[loc.packet_index] + loc.offset, arg_list);
                            if(ret != SUCCESS) {
                                logger.log_message("calculation failed");
                                // do nothing
                            } else if(ret != NOCHANGE) {
                                // we changed a measurement in our buffer, mark it to be written to shared memory
                                output_packets.insert(loc.packet_index);
                            } // else convert function said not to write anything but didn't fail
                        }
                    }
                }
            }

            triggers_processed.clear();
        }

        // write out any changed packets to shared memory
        for(uint32_t packet_index : output_packets) {
            if(tshm.write(packet_index, packet_buffers[packet_index]) != SUCCESS) {
                logger.log_message("failed to write buffer to shared memory");
                // do nothing
            }

            // always log packets
            if(loggers[packet_index]->log_packet(packet_buffers[packet_index], packet_sizes[packet_index]) != SUCCESS) {
                logger.log_message("failed to log virtual packet to filesystem");
                // do nothing
            }
        }

        // rinse and repeat
    }
}
