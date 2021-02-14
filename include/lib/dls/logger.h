/**
*   Base Logger class. Used to write messages to POSIX mqueues.
**/
#ifndef LOGGER_H
#define LOGGER_H

#include "common/types.h"
#include <string>
#include <mqueue.h>
#include <sys/time.h>

namespace dls {

    static const size_t MAX_Q_SIZE = 2048;

    class Logger {
    public:
        Logger(std::string queue_name);
        ~Logger();
        RetType Open();
        RetType Close();
        RetType queue_msg(const char* buffer, size_t size);
        bool open;
        struct timeval curr_time;
    private:
        std::string queue_name;
        mqd_t mq;
    };
}
#endif
