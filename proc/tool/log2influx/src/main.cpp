/********************************************************************
*  Name: main.cpp
*
*  Purpose: Uploads log files to InfluxDB, assumes InfluxDB server is hosted
*           at domain name 'influx.local'
*
*  Usage: ./log2influx [log file directory] (vcm config file path)
*         vcm config file path is optional, uses the default if not set
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include "lib/vcm/vcm.h"
#include "lib/dls/dls.h"
#include "lib/convert/convert.h"
#include "common/types.h"
#include "common/net.h"

#include <string>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define INFLUXDB_HOST "influx.local"
#define INFLUXDB_UDP_PORT 8089

using namespace vcm;
using namespace dls;
using namespace convert;


int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("usage: ./log2influx [log file directory] (vcm config file path)");
        return -1;
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

    // create network socket
    int sockfd = -1;
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
       printf("socket creation failed\n");
       return -1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(INFLUXDB_UDP_PORT);

    if(FAILURE == get_addr(INFLUXDB_HOST, &servaddr.sin_addr)) {
        printf("failed to resolve host name: %s\n", INFLUXDB_HOST);
        return -1;
    }

    // open the first input file
    std::string base_filename = dir;
    base_filename += "/telemetry.log";
    int file_index = 0;

    std::string filename = base_filename;

    struct timeval t0 = {0, 0};

    printf("uploading logged measurements to InfluxDB\n");

    while(1) {
        std::ifstream f(filename.c_str(), std::ifstream::binary);

        // file doesn't exist
        if(!f) {
            printf("No input file: %s\n", filename.c_str());
            break;
        }

        printf("uploading file: %s\n", filename.c_str());

        packet_record_t* rec;
        while(!f.eof()) {
            rec = retrieve_record(f);

            if(NULL == rec) {
                printf("Unexpected failure to parse record in file: %s\n", filename.c_str());
                continue;
            }

            // set to some large number, want it to break if scanf has a bad string
            uint32_t packet_id = 1 << 31;

            size_t fst = rec->device->find('(');
            size_t second = rec->device->find(')');
            if(fst == std::string::npos || second == std::string::npos) {
                printf("packet id not found in device name in file: %s\n", filename.c_str());
            }

            std::string id_str = rec->device->substr(fst, second);
            if(EOF == sscanf(id_str.c_str(), "(%u)", &packet_id)) {
                printf("scanf error in file: %s\n", filename.c_str());

                // look for the next record
                continue;
            }

            if(packet_id > veh->num_packets - 1) {
                printf("invalid packet id parsed from device name\n");

                // look for the next record
                continue;
            }

            if(rec->size != veh->packets[packet_id]->size) {
                printf("size mismatch in file: %s\n", filename.c_str());

                // look for the next record
                continue;
            }

            std::string val;
            measurement_info_t* meas = NULL;
            uint8_t* data = NULL;
            std::unordered_map<std::string, size_t>* packet_map = &(packets[packet_id]);
            uint8_t first = 1;

            // write the measurement name
            std::string msg = veh->device;
            msg += " ";

            for(std::string m : veh->measurements) {
                if(packet_map->find(m) != packet_map->end()) {
                    // we found a measurement in this record
                    val = "";
                    meas = veh->get_info(m);
                    data = rec->data + (*packet_map)[m];

                    if(convert_to(veh, meas, data, &val) != SUCCESS) {
                        printf("failed to convert value in file: %s\n", filename.c_str());
                        val = "";
                        // try the next measurement
                        continue;
                    }

                    // write each field
                    if(!first) {
                        msg += ",";
                        first = 0;
                    }

                    msg += m;
                    msg += "=";
                    msg += val;
                }
            }

            // if this is our first data point, record the start timestamp
            if(t0.tv_sec == 0 && t0.tv_usec == 0) {
                t0 = rec->timestamp;
            }

            // calculate the timestamp relative to t0
            long int sec = rec->timestamp.tv_sec - t0.tv_sec;
            long int usec = rec->timestamp.tv_usec - t0.tv_usec;

            if(usec < 0) {
                sec--;
                usec = 1000000 + usec;
            }

            if(sec < 0) {
                // this should never happen
                printf("record occurred after first record, not uploading\n");
            } else {
                unsigned long int nanosec = (sec * 1000000000) + (usec * 1000);
                msg += " ";
                msg += std::to_string(nanosec);

                // send the message to the InfluxDB server
                ssize_t sent = -1;
                sent = sendto(sockfd, msg.c_str(), msg.length(), 0,
                    (struct sockaddr*)&servaddr, sizeof(servaddr));

                if(sent == -1) {
                    printf("failed to send UDP line protocol message\n");
                }
            }

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

    close(sockfd);
    printf("upload to InfluxDB complete\n");
}
