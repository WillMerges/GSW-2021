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

NetworkManager::NetworkManager(uint16_t port, const char* name, uint8_t* buffer, size_t size, ssize_t rx_timeout) {
    mqueue_name = "/";
    mqueue_name += name;

    rx_buffer = buffer;
    rx_size = size;

    this->port = port;
    this->rx_timeout = rx_timeout;

    open = false;

    // in_buffer = new char[vcm->packet_size];
    // in_size = 0;

    // if(SUCCESS != Open()) {
    //     throw new std::runtime_error("failed to open network manager");
    // }
}

NetworkManager::~NetworkManager() {
    // if(buffer) {
    //     delete[] buffer;
    // }

    // if(in_buffer) {
    //     delete[] in_buffer;
    // }

    if(open) {
        Close();
    }
}

RetType NetworkManager::Open() {
    MsgLogger logger("NetworkManager", "Open");

    if(open) {
        return SUCCESS;
    }

    // if size >= MAX_MSG_SIZE and we get a message greater than we can fit
    // in our buffer, in_size will be set to MAX_MSG_SIZE. If packet_size == MAX_MSG_SIZE
    // then we can't tell if we have a truncated packets or a valid one. If packet_size
    // is too large we can't store the whole packet regardless.
    /*
    if(vcm->packet_size >= MAX_MSG_SIZE) {
        logger.log_message("VCM packet size is greater than equal to max message \
                            size, cannot fit packet in allocated buffer");
        return FAILURE;
    }
    */
    // new change: this is totally fine, since the bufer is passed in now it's guaranteed to be the correct size
    // actually, this never made sense since our receiving buffer is dynamically allocated

    // open the mqueue
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = NM_MAX_MSG_SIZE; // this limits how many bytes we can send
    attr.mq_curmsgs = 0;

    // TODO we may want to block on this
    mq = mq_open(mqueue_name.c_str(), O_RDONLY|O_CREAT, 0644, &attr);
    if((mqd_t)-1 == mq) {
        logger.log_message("unable to open mqueue");
        return FAILURE;
    }

    // set up the socket
    if(rx_timeout < 0) { // blocking mode
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { // ipv4, UDP
           logger.log_message("socket creation failed");
           return FAILURE;
        }
    } else { // non-blocking
        if ((sockfd = socket(AF_INET, SOCK_DGRAM | O_NONBLOCK, 0)) < 0 ) { // ipv4, UDP
           logger.log_message("socket creation failed");
           return FAILURE;
        }
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

    // set the timeout if there is one, -1 means no timeout
    if(rx_timeout >= 0) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = rx_timeout;
        if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            logger.log_message("failed to set timeout on socket");
            return FAILURE;
        }
    }

    // zero address struct for the device (NOT the ground station)
    memset(&device_addr, 0, sizeof(device_addr));

    device_addr.sin_family = AF_INET;
    // we no longer set the port and address of the device
    // originally, we set these in case we call sendto before recvfrom
    // recvfrom fills in the port and address of the receiver for us,
    // but until it sends something we don't know it's port/address
    // so now we just error if it hasn't sent us anything yet when sendto is called
    //device_addr.sin_port = htons(vcm->port);
    //device_addr.sin_addr.s_addr = htons(vcm->addr);

    // TODO maybe don't call bind unless the user says to specifically
    struct sockaddr_in myaddr;
    myaddr.sin_addr.s_addr = htons(INADDR_ANY); // use any interface we have available (likely just 1 ip)
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(port); // bind OUR port (we receive and send from this port instead of letting the OS pick)
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

    // TODO check the NmShm to see if we have an address we can send to that
    // a receiving instance stripped, if we don't just return since we don't
    // know where to send
    // before unlocking anything, check a single byte that the receiving process
    // will set everytime it gets a packet.
    // The receiving end will just move on if the shm is locked, we don't want
    // to lock the process out by trying to read if we know we don't have anything
    // we can assume that a single byte is accessed atomically so we can just
    // check that it's set before locking
    // TODO this could still be an issue if we lock the receiving process out
    // of shared memory and the IP changes, maybe do a writers preference lock?
    // or just check a writers lock and a readers lock? if a reader locked it
    // the receiver should wait, if it was a writer it should just not care
    // or just don't care and don't expect the IP to change, and if it does we
    // shouldn't call send

    // ^^^ call the above before checking the mqueue, so we don't drop any
    // messages if we already know we're going to get an error

    // check the mqueue
    ssize_t read = -1;
    read = mq_receive(mq, (char*)tx_buffer, NM_MAX_MSG_SIZE, NULL);

    // send the message from the mqueue out of the socket
    if(read != -1) {

        // if the port is still zeroed we haven't received a packet yet
        // which means we don't have a valid IP address to send to
        if(0 == device_addr.sin_port) {
            logger.log_message("Receiver has not yet sent a packet providing an"
                                " address, failed to send UDP message");
            return FAILURE;
        }

        // device_addr.sin_port = htons(vcm->port);

        ssize_t sent = -1;
        sent = sendto(sockfd, (char*)tx_buffer, read, 0,
            (struct sockaddr*)&device_addr, sizeof(device_addr)); // send to whatever we last received from
        if(sent == -1) {
            logger.log_message("Failed to send UDP message");
            return FAILURE;
        }

        return SUCCESS;
    }

    logger.log_message("Failed to retreieve message from mqueue");
    return FAILURE;
}

RetType NetworkManager::Receive(size_t* received) {
    MsgLogger logger("NetworkManager", "Receive");

    int n = -1;
    socklen_t len = sizeof(device_addr);

    // MSG_DONTWAIT should be taken care of by O_NONBLOCK (so actually don't use it anymore incase we are in blocking mode)
    // MSG_TRUNC is set so that we know if we overran our buffer
    n = recvfrom(sockfd, (char*)rx_buffer, NM_MAX_MSG_SIZE,
        MSG_TRUNC, (struct sockaddr *) &device_addr, &len); // fill in device_addr with where the packet came from

    if(n == -1) { // timeout or error
        received = 0;
        return FAILURE; // no packet
    }

    // TODO try and write the IP address of device_addr to NmShm
    // if it's locked we can just ignore it and move on

    // // set in size to the size of the buffer if we received too much data for our buffer
    // if(n > MAX_MSG_SIZE) {
    //     in_size = MAX_MSG_SIZE;
    // } else {
    //     in_size = n;
    // }

    *received = n;
    return SUCCESS;
}

NetworkInterface::NetworkInterface(const char* name) {
    mqueue_name = "/";
    mqueue_name += name;

    // try to open, do nothing if it doesn't work (don't want to blow everything up)
    // Open();
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
    mq = mq_open(mqueue_name.c_str(), O_WRONLY|O_NONBLOCK); // TODO consider adding O_CREAT here, or actually should it fail if the network manager doesnt exist?
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
        logger.log_message("Network Interface not open");
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
    MsgLogger logger("NetworkInterface", "QueueUDPMessage");

    if(!open) {
        logger.log_message("Network Interface not open");
        return FAILURE;
    }

    if(size > NM_MAX_MSG_SIZE) {
        logger.log_message("Message too large");
        return FAILURE;
    }

    if(0 > mq_send(mq, msg, size, 0)) {
        logger.log_message("Failed to queue message");
        return FAILURE;
    }

    return SUCCESS;
}

// #undef MAX_MSG_SIZE
// #undef RECV_TIMEOUT
