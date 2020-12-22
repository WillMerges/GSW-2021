#include "lib/dls/packet_logger.h"
#include <string.h>
#include <string>

using namespace dls;

PacketLogger::PacketLogger(std::string device_name):
                                            Logger(TELEMETRY_MQUEUE_NAME) {
    this->device_name = device_name;
}

RetType PacketLogger::log_packet(unsigned char* buffer, size_t size) {
    std::string preamble = device_name + "<";

    size_t out_size = size + preamble.size() + 1;
    char* out_buff = new char[out_size];

    memcpy(out_buff, preamble.c_str(), preamble.size());
    memcpy(out_buff+preamble.size(), buffer, size);
    out_buff[out_size - 1] = '>';

    RetType ret = queue_msg(out_buff, out_size);
    delete[] out_buff;
    return ret;
}
