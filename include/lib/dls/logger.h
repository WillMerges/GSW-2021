/**
*   Base Logger class. Used to write messages to POSIX mqueues.
**/

#ifndef LOGGER_H
#define LOGGER_H

#include "common/types.h"
#include <string>
#include <mqueue.h>

namespace dls {
    class Logger {
    public:
        Logger(std::string queue_name);
        RetType Open();
        RetType Close();
        RetType queue_msg(const char* buffer, size_t size);
    private:
        std::string queue_name;
        mqd_t mq;
        bool open;
    };
}
#endif
