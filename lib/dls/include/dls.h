#ifndef DLS_H
#define DLS_H

#include "types.h"
#include <string>

namespace dls {

    static const char* MQUEUE_NAME = "/log_queue"; // TODO change to some tmp location later

    class MsgLogger {
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
