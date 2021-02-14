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

MsgLogger::MsgLogger(std::string func_name): Logger(MESSAGE_MQUEUE_NAME) {
    this->func_name = func_name;
    this->class_name = "";
}

// logged messages look like
// | [timestamp] | (class name, function name) | message |
RetType MsgLogger::log_message(std::string msg) {
    gettimeofday(&curr_time, NULL);
    std::string new_msg = "[" + std::to_string(curr_time.tv_sec) + "] ";

    new_msg += "("+class_name;
    if(class_name != "" && func_name != "") {
        new_msg += ", ";
    }
    new_msg += func_name;
    new_msg += ") ";
    new_msg += msg;

    return queue_msg(new_msg.c_str(), new_msg.size());
}
