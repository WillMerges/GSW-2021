/**
*   Queues messages to the telemetry packet queue. Queues are POSIX mqueues.
*   The message writer process will read the queue and write the packets to
*   the correcty log files atomically.
**/

#ifndef PACKET_LOGGER_H
#define PACKET_LOGGER_H

#include "common/types.h"
#include "lib/dls/logger.h"
#include <string>
#include <iostream>

namespace dls {

    static const char* const TELEMETRY_MQUEUE_NAME = "/telemetry_queue";

    typedef struct {

    } timestamp_t;

    typedef struct {
        struct timeval timestamp;
        std::string* device;
        size_t size;
        uint8_t* data;
    } packet_record_t;

    class PacketLogger : public Logger {
    public:
        // create a new packet logger
        PacketLogger(std::string device_name);

        // log a packet
        RetType log_packet(unsigned char* buffer, size_t size);
    private:
        std::string device_name;
    };

    // pulls a packet out of a log file
    // f's seek pointer should be at the start of a new logged packet
    // returns NULL on failure
    // to free memory from a packet_record_t, call 'free_record'
    packet_record_t* retrieve_record(std::istream& f);

    // frees the memory used by a packet record
    void free_record(packet_record_t* packet);
}

#endif
