#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <exception>
#include <unistd.h>
#include "lib/nm/nm.h"
#include "lib/dls/dls.h"
#include "lib/shm/shm.h"
#include "common/types.h"

using namespace nm;
using namespace dls;
using namespace shm;
using namespace vcm;

#define MAX_MSG_SIZE 2048
#define RECV_TIMEOUT 100000 // 100ms

NetworkManager::NetworkManager(VCM* vcm) {
    mqueue_name = "/";
    mqueue_name += vcm->device;

    this->vcm = vcm;

    buffer = new char[MAX_MSG_SIZE];

    open = false;

    in_buffer = new char[vcm->packet_size];
    in_size = 0;

    // if(SUCCESS != Open()) {
    //     throw new std::runtime_error("failed to open network manager");
    // }
}

NetworkManager::~NetworkManager() {
    if(buffer) {
        delete[] buffer;
    }

    if(in_buffer) {
        delete[] in_buffer;
    }

    if(open) {
        Close();
    }
}

RetType NetworkManager::Open() {
    MsgLogger logger("NetworkManager", "Open");

    if(open) {
        return SUCCESS;
    }

    // open the mqueue
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    mq = mq_open(mqueue_name.c_str(), O_RDONLY|O_NONBLOCK|O_CREAT, 0644, &attr);
    if((mqd_t)-1 == mq) {
        logger.log_message("unable to open mqueue");
        return FAILURE;
    }

    // set up the socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
       logger.log_message("socket creation failed");
       return FAILURE;
    }

    // at this point we need to close the socket regardless
    open = true;

    // TODO this is (hopefully) just for simulation
    // related release notes -> https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=c617f398edd4db2b8567a28e899a88f8f574798d
    int on = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        logger.log_message("failed to set socket to reusreaddr");
        return FAILURE;
    }

    on = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
        logger.log_message("failed to set socket to reuseport");
        return FAILURE;
    }

    // set the timeout
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = RECV_TIMEOUT;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        logger.log_message("failed to set timeout on socket");
        return FAILURE;
    }

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(vcm->port);
    servaddr.sin_addr.s_addr = htons(INADDR_ANY); // TODO this needs to be a specific IP of the receiver

    struct sockaddr_in myaddr;
    myaddr.sin_addr.s_addr = htons(INADDR_ANY);
    myaddr.sin_family = AF_INET;
    // myaddr.sin_port = htons(GSW_PORT); // TODO maybe use multiple ports for different devices?
    myaddr.sin_port = htons(vcm->port); // use the same port for the server and client
    int rc = bind(sockfd, (struct sockaddr*) &myaddr, sizeof(myaddr));
    if(rc) {
        logger.log_message("socket bind failed");
        return FAILURE;
    }

    return SUCCESS;
}

RetType NetworkManager::Close() {
    MsgLogger logger("NetworkManager", "Close");

    RetType ret = SUCCESS;

    if(!open) {
        ret = FAILURE;
        logger.log_message("nothing to close, network manager not open");
    }

    if(0 != mq_close(mq)) {
        ret = FAILURE;
        logger.log_message("unable to close mqueue");
    }

    if(0 != close(sockfd)) {
        ret = FAILURE;
        logger.log_message("unable to close socket");
    }

    return ret;
}

RetType NetworkManager::Send() {
    MsgLogger logger("NetworkManager", "Send");

    if(!open) {
        logger.log_message("network manager not open");
        return FAILURE;
    }

    // check the mqueue
    ssize_t read = -1;
    read = mq_receive(mq, buffer, MAX_MSG_SIZE, NULL);

    // send the message from the mqueue out of the socket
    if(read != -1) {
        ssize_t sent = -1;
        sent = sendto(sockfd, buffer, read, 0,
            (struct sockaddr*)&servaddr, sizeof(servaddr));
        if(sent == -1) {
            logger.log_message("Failed to send UDP message");
            return FAILURE;
        }
    }

    return SUCCESS;
}

RetType NetworkManager::Receive() {
    MsgLogger logger("NetworkManager", "Receive");

    int n = -1;
    socklen_t len = sizeof(servaddr);

    // MSG_DONTWAIT should be taken care of by O_NONBLOCK
    // MSG_TRUNC is set so that we know if we have a size mismatch
    n = recvfrom(sockfd, in_buffer, vcm->packet_size,
                MSG_DONTWAIT | MSG_TRUNC, (struct sockaddr *) &servaddr, &len);

    if(n == -1) { // timeout or error
        in_size = 0;
        return FAILURE; // no packet
    }

    in_size = n;
    return SUCCESS;
}

NetworkInterface::NetworkInterface(VCM* vcm) {
    mqueue_name = "/";
    mqueue_name += vcm->device;

    // try to open, do nothing if it doesn't work (don't want to blow everything up)
    Open();
}

NetworkInterface::~NetworkInterface() {
    Close();
}

RetType NetworkInterface::Open() {
    MsgLogger logger("NetworkInterface", "Open");

    if(open) {
        return SUCCESS;
    }

    // turned non blocking on so if the queue is full it won't be logged (could be a potential issue if messages are being dropped)
    mq = mq_open(mqueue_name.c_str(), O_WRONLY|O_NONBLOCK); // TODO consider adding O_CREAT here
    if((mqd_t)-1 == mq) {
        logger.log_message("unable to open mqueue");
        return FAILURE;
    }

    open = true;
    return SUCCESS;
}

RetType NetworkInterface::Close() {
    MsgLogger logger("NetworkInterface", "Close");

    if(!open) {
        return SUCCESS;
    }

    if((mqd_t)-1 == mq_close(mq)) {
        logger.log_message("failed to close mqueue");
        return FAILURE;
    }

    open = false;

    return SUCCESS;
}

RetType NetworkInterface::QueueUDPMessage(const char* msg, size_t size) {
    if(!open) {
        return FAILURE;
    }

    if(size > MAX_MSG_SIZE){
        return FAILURE;
    }

    if(0 > mq_send(mq, msg, size, 0)) {
        return FAILURE;
    }

    return SUCCESS;
}

#undef MAX_MSG_SIZE
#undef RECV_TIMEOUT
