#include "lib/dls/packet_logger.h"
#include <string.h>
#include <string>
#include <stdint.h>

using namespace dls;

PacketLogger::PacketLogger(std::string device_name):
                                            Logger(TELEMETRY_MQUEUE_NAME) {
    this->device_name = device_name;
}

// logged packets look like:
// | [timestamp] | <device_name> | size of packet | packet data |
RetType PacketLogger::log_packet(unsigned char* buffer, size_t size) {
    gettimeofday(&curr_time, NULL);
    std::string header = "[" + std::to_string(curr_time.tv_sec) + "." +
                               std::to_string(curr_time.tv_usec) + "]";

    header += "<" + device_name + ">";
    size_t len = strlen(header.c_str()) * sizeof(char); // want to get actual # of bytes

    size_t out_size = len + size + sizeof(size_t);

    char* out_buff = new char[out_size];

    memcpy(out_buff, header.c_str(), len); // write the header
    memcpy(out_buff+len, &size, sizeof(size_t)); // write the packet size
    memcpy(out_buff+len+sizeof(size_t), buffer, size); // write the packet

    RetType ret = queue_msg(out_buff, out_size);
    delete[] out_buff;

    return ret;
}

packet_record_t* dls::retrieve_record(std::istream& f) {
    // get the timestamp
    std::string* timestamp = new std::string();

    char c = 0;
    bool done = false;
    while(!done) {
        if(!f.read(&c, sizeof(char))) {
            return NULL;
        }

        // some error or EOF
        // if(!f) {
        //     return NULL;
        // }

        if(c == '[') {
            // we found a start of a record in the file
            while(f) {
                f.read(&c, sizeof(char));

                if(c == ']') {
                    // got the timestamp
                    break;
                }

                *timestamp += c;
                done = true;
            }
        }
    }

    // get the device name
    std::string* device = new std::string();

    f.read(&c, sizeof(char));
    if(!f || c != '<') {
        return NULL;
    }

    while(1) {
        f.read(&c, sizeof(char));

        if(!f) {
            return NULL;
        }

        if(c == '>') {
            // all done
            break;
        }

        *device += c;
    }

    // get the size of the packet
    uint8_t buff[sizeof(size_t)];
    f.read((char*)buff, sizeof(size_t));

    if(!f) {
        return NULL;
    }

    size_t size = *((size_t*)buff);

    // read the rest of the packet
    uint8_t* data = (uint8_t*)malloc(size);
    f.read((char*)data, size);

    if(!f) {
        return NULL;
    }

    // put together the record to return
    packet_record_t* rec = (packet_record_t*)malloc(sizeof(packet_record_t));
    rec->timestamp = timestamp;
    rec->device = device;
    rec->size = size;
    rec->data = data;

    return rec;
}

void dls::free_record(packet_record_t* packet) {
    if(!packet) {
        // nothing to free, NULL pointer
        return;
    }

    if(packet->timestamp != NULL) {
        delete packet->timestamp;
    }

    if(packet->device != NULL) {
        delete packet->device;
    }

    if(packet->data != NULL) {
        free(packet->data);
    }

    free(packet);
}
