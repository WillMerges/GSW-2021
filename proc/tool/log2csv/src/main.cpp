/********************************************************************
*  Name: main.cpp
*
*  Purpose: Parses log files into a single CSV file of measurements
*
*  Usage: ./log2csv [log file directory] (vcm config file path)
*         vcm config file path is optional, uses the default if not set
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/

#include "lib/vcm/vcm.h"
#include "lib/dls/dls.h"

using namespace vcm;
using namespace dls;

int main(int argc, char** argv) {
    if(argc < 2) {
        printf("usage: ./log2csv [log file directory] (vcm config file path)");
    }

    char* dir = argv[1];

    VCM* veh;
    if(argc > 2) {
        veh = new VCM(argv[2]);
    } else {
        // use default config file location
        veh = new VCM();
    }

    // initialize VCM (reads log file)
    if(veh->init() != SUCCESS) {
        printf("failed to initialize vcm");
        return -1;
    }

    // stores list of measurements for each packet id
    // assumes packet id's are sequential and ascending by 1
    std::vector<std::vector<measurement_info_t>> packets;

    std::vector<measurement_info_t> temp;
    measurement_info_t* meas = NULL;

    // find what measurements belong in each packet
    for(uint32_t i = 0; i < veh->num_packets; i++) {
        for(std::string s : veh->measurements) {
            meas = veh->get_info(s);
            for(location_info_t loc : meas->locations) {
                if(loc.packet_index == i) {
                    // this measurement is in packet with index i
                    temp.push_back(*meas);
                }
            }
        }

        packets.push_back(temp);
        temp.clear();
    }

    // TODO
    // go through each file in directory
    // open file, call retrieve_record on packet
    // for each record, get which packet id, then extract all measurements
    // then write each measurement to CSV file
}
