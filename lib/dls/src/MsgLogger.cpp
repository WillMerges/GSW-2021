#include "message_logger.h"

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
    // TODO add timestamp? (maybe let the writing process do this)
    std::string new_msg = class_name + ", " + func_name + ": " + msg;

    return queue_msg(msg.c_str(), msg.size());
}
