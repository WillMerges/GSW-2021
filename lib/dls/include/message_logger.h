/**
*   Queues messages to the system message queue. Queues are POSIX mqueues.
*   The message writer process will read the queue and write the packets to
*   the correcty log files atomically.
**/

#ifndef MESSAGE_LOGGER_H
#define MESSAGE_LOGGER_H

#include "types.h"
#include "logger.h"
#include <string>

namespace dls {

    static const char* MESSAGE_MQUEUE_NAME = "/log_queue";

    class MsgLogger : public Logger {
    public:
        MsgLogger(std::string class_name, std::string func_name);
        MsgLogger(std::string func_name);
        MsgLogger();
        RetType log_message(std::string msg);
    private:
        std::string class_name;
        std::string func_name;
    };
}

#endif
