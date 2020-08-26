#include "lib/dls/packet_logger.h"
#include <string.h>

using namespace dls;

PacketLogger::PacketLogger(std::string device_name):
                                            Logger(TELEMETRY_MQUEUE_NAME) {
    this->device_name = device_name;
}

RetType PacketLogger::log_packet(unsigned char* buffer, size_t size) {
    // TODO add timestamp? (maybe let the writing process do this)
    std::string preamble = device_name + ">";
    size_t out_size = size + preamble.size();
    char* out_buff = new char[out_size];
    memcpy(out_buff, buffer, size);
    memcpy(out_buff+size, preamble.c_str(), preamble.size());

    return queue_msg(out_buff, out_size);
}
