#include "lib/dls/logger.h"
#include "common/types.h"
#include <stdexcept>

using namespace dls;

Logger::Logger(std::string queue_name): open(false) {
    this->queue_name = queue_name;
    if(Open() != SUCCESS) {
        throw new std::runtime_error("Failed to open logger");
    }
}

Logger::~Logger() {
    Close();
}

RetType Logger::Open() {
    if(open) {
        return SUCCESS;
    }

    mq = mq_open(queue_name.c_str(), O_WRONLY);
    if((mqd_t)-1 == mq) {
        // no sense in logging this because it is the logger!
        return FAILURE;
    }
    open = true;
    return SUCCESS;
}

RetType Logger::Close() {
    if((mqd_t)-1 == mq_close(mq)) {
        std::string err = "Failed to close mqueue " + queue_name;
        for(int i = 0; i<5; i++) { // try to send error msg 5 times
            if(0 <= mq_send(mq, err.c_str(), err.size(), 0)) {
                return FAILURE;
            }
        }
    }
    open = false;
    return FAILURE;
}

RetType Logger::queue_msg(const char* buffer, size_t size) {
    if(size > MAX_Q_SIZE) {
        return FAILURE;
    }

    if(!open) {
        return FAILURE;
    }

    if(0 > mq_send(mq, buffer, size, 0)) {
        // no sense in logging this because it is the logger!
        return FAILURE;
    }
    return SUCCESS;
}
