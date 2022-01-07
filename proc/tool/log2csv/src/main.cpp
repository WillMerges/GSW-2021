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
#include "lib/convert/convert.h"
#include <string>
#include <vector>
#include <unordered_map>

using namespace vcm;
using namespace dls;
using namespace convert;

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
    // measurement names map to offset in packet
    // assumes packet id's are sequential and ascending by 1
    std::vector<std::unordered_map<std::string, size_t>> packets;

    std::unordered_map<std::string, size_t> temp;
    measurement_info_t* meas = NULL;

    // find what measurements belong in each packet
    for(uint32_t i = 0; i < veh->num_packets; i++) {
        for(std::string s : veh->measurements) {
            meas = veh->get_info(s);
            for(location_info_t loc : meas->locations) {
                if(loc.packet_index == i) {
                    // this measurement is in packet with index i
                    temp[s] = loc.offset;
                }
            }
        }

        packets.push_back(temp);
        temp.clear();
    }

    // open the output file
    std::ofstream of;
    std::string of_name = dir;
    of_name += "/log.csv";
    of.open(of_name.c_str(), std::ios::out | std::ios::trunc);

    if(!of.is_open()) {
        printf("Failed to open output file: %s\n", of_name.c_str());
        return -1;
    }

    // write out the first entry
    of << "timestamp,packet id,";
    for(std::string m : veh->measurements) {
        of << m << ",";
    }
    of << '\n';
    of.flush();

    // open the first input file
    std::string base_filename = dir;
    base_filename += "/telemetry.log";
    int file_index = 0;

    std::string filename = base_filename;

    while(1) {
        std::ifstream f(filename.c_str(), std::ifstream::binary);

        // file doesn't exist
        if(!f) {
            printf("No input file: %s\n", filename.c_str());
            break;
        }

        // f.open(filename.c_str(), std::ifstream::binary);

        // if(!f.is_open()) {
        //     break;
        // }

        packet_record_t* rec = NULL;

        // should hit eof on last case
        while(!f.eof()) {
            rec = retrieve_record(f);

            if(rec == NULL) {
                // something bad happened, try and parse the next record
                printf("Unexpected failure to parse record in file: %s\n", filename.c_str());
                continue;
            }

            // set to some large number, want it to break if scanf has a bad string
            uint32_t packet_id = 1 << 31;

            size_t first = rec->device->find('(');
            size_t second = rec->device->find(')');
            if(first == std::string::npos || second == std::string::npos) {
                printf("packet id not found in device name in file: %s\n", filename.c_str());
            }

            std::string id_str = rec->device->substr(first, second);
            if(EOF == sscanf(id_str.c_str(), "(%u)", &packet_id)) {
                printf("scanf error in file: %s\n", filename.c_str());

                // look for the next record
                continue;
            }

            if(rec->size != veh->packets[packet_id]->size) {
                printf("size mismatch in file: %s\n", filename.c_str());
                break;
            }

            // write out the record to the CSV file
            of << rec->timestamp->c_str() << "," << std::to_string(packet_id) << ",";

            std::string val;
            measurement_info_t* meas = NULL;
            uint8_t* data = NULL;
            std::unordered_map<std::string, size_t>* packet_map = NULL;

            for(std::string m : veh->measurements) {
                val = "";

                packet_map = &(packets[packet_id]);
                if(packet_map->find(m) != packet_map->end()) {
                    // this measurement is in this record, add it to the csv
                    meas = veh->get_info(m);
                    data = rec->data + (*packet_map)[m];

                    if(convert_to(veh, meas, data, &val) != SUCCESS) {
                        printf("failed to convert value in file: %s\n", filename.c_str());
                        val = "";
                        // try the next measurement
                        continue;
                    }
                }

                // NOTE: we leave an extra comma on each line, but who cares
                of << val << ',';
                of.flush();
            }

            of << '\n';
            of.flush();

            free_record(rec);

            // causes the EOF flag to be set if the next character is eof
            // this should happen on the last packet unless we mess something up
            // if we did mess something up, the conversion should fail at some point
            // and then we drop the file and move on
            f.peek();
        }

        f.close();

        // make the name for the next file
        filename = base_filename;
        file_index++;
        filename += std::to_string(file_index);
    }

    of.close();

    printf("completed parsing\n");
    printf("file written to: %s\n", of_name.c_str());
}
