#include "lib/dls/logger.h"
#include "common/types.h"
#include <stdexcept>

using namespace dls;

Logger::Logger(std::string queue_name): open(false) {
    this->queue_name = queue_name;
    // if(Open() != SUCCESS) {
    //     throw new std::runtime_error("Failed to open logger");
    // }
    Open(); // don't kill everything if the logger doesn't work
}

Logger::~Logger() {
    Close(); // don't care if this works
}

RetType Logger::Open() {
    if(open) {
        return SUCCESS;
    }

    // turned non blocking on so if the queue is full it won't be logged (could be a potential issue if messages are being dropped)
    // turned off O_NONBLOCK
    mq = mq_open(queue_name.c_str(), O_WRONLY); // TODO consider adding O_CREAT here
    if((mqd_t)-1 == mq) {
        // no sense in logging this because it is the logger!
        return FAILURE;
    }
    open = true;
    return SUCCESS;
}

RetType Logger::Close() {
    if(!open) {
        return SUCCESS;
    }

    if((mqd_t)-1 == mq_close(mq)) {
        std::string err = "Failed to close mqueue: " + queue_name;
        for(int i = 0; i<5; i++) { // try to send error msg 5 times
            if(0 == mq_send(mq, err.c_str(), err.size(), 0)) {
                open = false;
                return SUCCESS;
            }
        }
    }
    return FAILURE;
}

RetType Logger::queue_msg(const char* buffer, size_t size) {
    if(!open) { // can't queue if it's not open
        return FAILURE;
    }

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
