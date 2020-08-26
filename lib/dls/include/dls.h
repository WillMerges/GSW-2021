#ifndef DLS_H
#define DLS_H

// Sends data to Mqueues (system messages and raw telemtry data) //TODO update documentation

#include "types.h"
#include <string>
#include <mqueue.h>

#define MSG_MQUEUE_NAME "/log_queue"
#define TELEMETRY_MQUEUE_NAME "/telemetry_queue"

namespace dls {

    // TODO causing unused variable warnings (make a separate constants class? or make separate headers?)
    //static const char* MSG_MQUEUE_NAME = "/log_queue"; // TODO change to some tmp location later
    //static const char* TELEMETRY_MQUEUE_NAME = "/telemetry_queue";

    // TODO make an inherited logger class

    // log system messages
    class MsgLogger {
    public:
        MsgLogger(std::string class_name, std::string func_name);
        MsgLogger(std::string func_name);
        MsgLogger();
        RetType Open();
        RetType Close();
        RetType log_message(std::string msg);
    private:
        std::string class_name;
        std::string func_name;
        mqd_t mq;
        bool open;
    };

    // log raw telemetry data from device
    class PacketLogger {
    public:
        PacketLogger(std::string device_name);
        RetType Open();
        RetType Close();
        RetType log_packet(unsigned char* buffer, size_t size);
    private:
        std::string device_name;
        mqd_t mq;
        bool open;
    };
}

#endif
