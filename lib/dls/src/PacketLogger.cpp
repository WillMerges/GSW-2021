#include "lib/dls/packet_logger.h"
#include <string.h>
#include <string>

using namespace dls;

PacketLogger::PacketLogger(std::string device_name):
                                            Logger(TELEMETRY_MQUEUE_NAME) {
    this->device_name = device_name;
}

// logged packets look like:
// | <device_name> | size of packet | packet data |
RetType PacketLogger::log_packet(unsigned char* buffer, size_t size) {
    // std::string preamble = device_name + "<";
    //
    // size_t out_size = size + preamble.size() + 1;
    // char* out_buff = new char[out_size];
    //
    // memcpy(out_buff, preamble.c_str(), preamble.size());
    // memcpy(out_buff+preamble.size(), buffer, size);
    // out_buff[out_size - 1] = '>';
    //
    // RetType ret = queue_msg(out_buff, out_size);
    // delete[] out_buff;

    std::string dev_id = "<" + device_name + ">";
    size_t len = strlen(dev_id.c_str()) * sizeof(char); // want to get actual # of bytes

    size_t out_size = len + size + sizeof(size_t);

    char* out_buff = new char[out_size];

    memcpy(out_buff, dev_id.c_str(), len); // write the device id
    memcpy(out_buff, &size, sizeof(size_t)); // write the packet size
    memcpy(out_buff+len+sizeof(size_t), buffer, size); // write the packet

    RetType ret = queue_msg(out_buff, out_size);
    delete[] out_buff;

    return ret;
}
