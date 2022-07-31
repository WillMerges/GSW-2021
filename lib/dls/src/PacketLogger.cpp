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
// NOTE: timestamp is written as a raw 'struct timeval'
// TODO don't use malloc, use a static buffer since we already limit by mqueue size
RetType PacketLogger::log_packet(unsigned char* buffer, size_t size) {
    gettimeofday(&curr_time, NULL);

    static char out_buff[MAX_Q_SIZE];

    // create the header with the device name
    std::string header = "<" + device_name + ">";
    size_t len = strlen(header.c_str()) * sizeof(char); // want to get actual # of bytes

    size_t out_size = 2*sizeof(char) + sizeof(curr_time) + len + size + sizeof(size_t);

    char* ptr = out_buff;

    // write timestamp
    memcpy(ptr, "[", sizeof(char));
    ptr += sizeof(char);

    memcpy(ptr, (void*)&curr_time, sizeof(curr_time));
    ptr += sizeof(curr_time);

    memcpy(ptr, "]", sizeof(char));
    ptr += sizeof(char);

    // write the header
    memcpy(ptr, header.c_str(), len);
    ptr += len;

    // write the packet size
    memcpy(ptr, &size, sizeof(size_t));
    ptr += sizeof(size_t);

    // write the packet
    memcpy(ptr, buffer, size);

    RetType ret = queue_msg(out_buff, out_size);

    return ret;
}

packet_record_t* dls::retrieve_record(std::istream& f) {
    // get the timestamp
    uint8_t t_buff[sizeof(struct timeval)];
    size_t i = 0;

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
                    done = true;
                    break;
                }

                t_buff[i++] = c;

                if(i > sizeof(struct timeval)) {
                    // timestamp too long
                    return NULL;
                }
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
    rec->timestamp = *((struct timeval*)(t_buff));
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

    if(packet->device != NULL) {
        delete packet->device;
    }

    if(packet->data != NULL) {
        free(packet->data);
    }

    free(packet);
}
