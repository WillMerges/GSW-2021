#include "lib/dls/message_logger.h"
#include <string>

using namespace dls;

MsgLogger::MsgLogger(): Logger(MESSAGE_MQUEUE_NAME), class_name(""),
                                                     func_name("") {}

MsgLogger::MsgLogger(std::string class_name, std::string func_name):
                                                Logger(MESSAGE_MQUEUE_NAME) {
    this->class_name = class_name;
    this->func_name = func_name;
}

MsgLogger::MsgLogger(std::string func_name): Logger(MESSAGE_MQUEUE_NAME),
                                                             class_name("") {
    this->func_name = func_name;
}

RetType MsgLogger::log_message(std::string msg) {
    std::string new_msg = "";
    if(class_name != "") {
        new_msg += "("+class_name;
    }
    if(func_name != "") {
        if(new_msg != "") {
            new_msg += ", ";
        }
        new_msg += func_name;
    }
    if(new_msg != "") {
        new_msg += ") ";
    }

    new_msg += msg;

    return queue_msg(new_msg.c_str(), new_msg.size());
}
