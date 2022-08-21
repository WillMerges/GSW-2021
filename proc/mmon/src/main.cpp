/********************************************************************
*  Name: main.cpp
*
*  Purpose: Measurement Monitor process, waits for updates the measurements
*           and executes trigger function if available
*
*  Usage:
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include "lib/trigger/trigger.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/telemetry/TelemetryWriter.h"
#include "lib/dls/dls.h"
#include "lib/vcm/vcm.h"

#include <stdint.h>
#include <signal.h>
#include <vector>
#include <unordered_set>
#include <boost/interprocess/offset_ptr.hpp>


using namespace dls;
using namespace vcm;
using namespace trigger;

std::shared_ptr<VCM> veh;
TelemetryShm* tshm;
std::shared_ptr<TelemetryViewer> tv;
std::shared_ptr<TelemetryWriter> tw;

bool killed = false;

void sighandler(int) {
    tv->sighandler();
    killed = true;
}

namespace std {
    template<>
    struct hash<trigger_t> {
        inline size_t operator()(const trigger_t& t) const {
            return t.unique_id;
        }
    };
}

int main(int argc, char** argv) {
    MsgLogger logger("MMON", "main");
    logger.log_message("starting mmon process");

    // interpret the 1st argument as a config_file location if available
    std::string config_file = "";
    if(argc > 1) {
        config_file = argv[1];
    }

    if(config_file == "") {
        veh = std::make_shared<VCM>(); // use default config file
    } else {
        veh = std::make_shared<VCM>(config_file); // use specified config file
    }

    // init VCM
    if(veh->init() != SUCCESS) {
        logger.log_message("failed to initialize VCM");
        return -1;
    }

    if(veh->trigger_file == "") {
        logger.log_message("no trigger file specified");
        return 1;
    }

    tshm = new TelemetryShm();

    // setup telemetry shm
    if(tshm->init(veh.get()) != SUCCESS) {
        logger.log_message("failed to init telemetry shm");
        return -1;
    }

    if(tshm->open() != SUCCESS) {
        logger.log_message("failed to open telemetry shm");
        return -1;
    }

    // setup telemetry viewer
    if(tv->init(veh, tshm)) {
        logger.log_message("failed to init telemetry viewer");
        return -1;
    }

    // setup telemetry viewer
    if(tw->init(veh, tshm)) {
        logger.log_message("failed to init telemetry viewer");
        return -1;
    }

    tv->set_update_mode(TelemetryViewer::BLOCKING_UPDATE);

    // parse the trigger file
    std::vector<trigger_t> triggers;
    if(SUCCESS != parse_trigger_file(veh.get(), &triggers)) {
        logger.log_message("failed to parse trigger file");
        return -1;
    }

    logger.log_message("successfully parsed trigger file");

    // add signal handlers
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGFPE, sighandler);
    signal(SIGABRT, sighandler);

    // packets that cause a trigger to be executed
    std::unordered_set<uint32_t> trigger_packets;

    // maps packet id to list of triggers to execute
    std::vector<std::unordered_set<trigger_t>> packet_map;

    for(size_t i = 0; i < veh->num_packets; i++) {
        std::unordered_set<trigger_t> l;
        packet_map.push_back(l);
    }

    for(trigger_t t : triggers) {
        // tv.add(t.meas);
        for(location_info_t loc : t.meas->locations) {
            trigger_packets.insert(loc.packet_index);
            packet_map[loc.packet_index].insert(t);
        }
    }

    // TODO add measurement that are triggers AND are arguments (we need to read them presumably)
    // TODO or should this be all so each function has access to every measurement?
    tv->add_all();

    // whether we should flush to shared memory
    uint8_t flush = 0;

    // main logic
    while(!killed) {
        if(SUCCESS != tv->update()) {
            // move on
            continue;
        }

        // lock packets for writing
        // don't want anyone writing to our virtual packets at the same time
        if(SUCCESS != tw->lock(false)) {
            continue;
        }

        // TODO can we parallelize some of this?
        // is the overhead worth it?
        for(uint32_t packet_id : trigger_packets) {
            if(tshm->updated[packet_id]) {
                // this packet updated, process it's triggers

                for(trigger_t t : packet_map[packet_id]) {
                    if(SUCCESS == t.func(tv.get(), tw.get(), &(t.args))) {
                        flush = 1;
                    }
                }
            }
        }

        // flush any updates
        if(flush) {
            tw->flush();
            flush = 0;
        }

        // unlock packets so others waiting can write to virtual packets
        // don't check return, continues anyways
        // TODO something bad probably happens if this errors, since we increment semaphore again
        tw->unlock();
    }

    return 1;
}
