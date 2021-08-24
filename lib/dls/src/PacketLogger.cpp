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
    uint32_t len = strlen(header.c_str()) * sizeof(char); // want to get actual # of bytes

    uint32_t out_size = len + size + sizeof(uint32_t);

    char* out_buff = new char[out_size];

    memcpy(out_buff, header.c_str(), len); // write the header
    memcpy(out_buff+len, &size, sizeof(size_t)); // write the packet size
    memcpy(out_buff+len+sizeof(size_t), buffer, size); // write the packet

    RetType ret = queue_msg(out_buff, out_size);
    delete[] out_buff;

    return ret;
}
