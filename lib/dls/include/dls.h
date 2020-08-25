#ifndef DLS_H
#define DLS_H

#include "types.h"
#include <string>

namespace dls {

    static const char* MQUEUE_NAME = "/log_queue"; // TODO change to some tmp location later

    class DLS {
    public:
        DLS(std::string class_name, std::string func_name);
        DLS(std::string func_name);
        DLS();
        RetType log_message(std::string msg);
    private:
        std::string class_name;
        std::string func_name;
    };
}

#endif
