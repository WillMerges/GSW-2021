/**
*   Queues messages to the telemetry packet queue.
*   The message writer process will read the queue and write the packets to
*   the correcty log files atomically.
**/

#ifndef PACKET_LOGGER_H
#define PACKET_LOGGER_H

#include "types.h"
#include "logger.h"
#include <string>

namespace dls {

    static const char* TELEMETRY_MQUEUE_NAME = "/telemetry_queue";

    class PacketLogger : public Logger {
    public:
        PacketLogger(std::string device_name);
        RetType log_packet(unsigned char* buffer, size_t size);
    private:
        std::string device_name;
    };

}

#endif
