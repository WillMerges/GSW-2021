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


NetworkReceiver::NetworkReceiver() {
    sockfd = -1;
    inited = false;
}

NetworkReceiver::~NetworkReceiver() {
    MsgLogger logger("NetworkReceiver", "destructor");

    if(sockfd != -1) {
        if(0 != close(sockfd)) {
            logger.log_message("unable to close socket");
        }
    }

    if(rx_buffer) {
        delete[] rx_buffer;
    }
}

RetType NetworkReceiver::init(uint16_t port, uint32_t multicast_addr, size_t rx_timeout, size_t buffer_size) {
    MsgLogger logger("NetworkReceiver", "init");

    if(inited) {
        logger.log_message("already initialized");
        return FAILURE;
    }

    // set up the socket
    if(rx_timeout <= 0) { // blocking mode
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

    // these options are needed hopefully just for simulation
    // allows us to reuse ports/addresses between processes
    // related release notes -> https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=c617f398edd4db2b8567a28e899a88f8f574798d
    int on = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        logger.log_message("failed to set socket reuseaddr option");
        return FAILURE;
    }

    on = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
        logger.log_message("failed to set socket reuseport option");
        return FAILURE;
    }

    // set the timeout if there is one, -1 means no timeout
    if(rx_timeout > 0) {
        struct timeval tv;
        tv.tv_sec = rx_timeout / 1000;
        tv.tv_usec = (rx_timeout % 1000) * 1000;

        if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            logger.log_message("failed to set timeout on socket");
            return FAILURE;
        }
    }

    // setup our address
    struct sockaddr_in myaddr;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY); // use any interface we have available (likely just 1 ip)
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(port); // bind to this port to receive from
    int rc = bind(sockfd, (struct sockaddr*) &myaddr, sizeof(myaddr));
    if(rc) {
        logger.log_message("socket bind failed");
        return FAILURE;
    }

    // if we have a multicast address, join the multicast group
    // this should an IGMP packet to the router (if there is one)
    // for a simple network like point to point, the packet is likely just dropped at the receiver
    if(multicast_addr != 0) {
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = multicast_addr;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            logger.log_message("failed to join multicast group");
            return FAILURE;
        }
    }

    // allocate memory for receive buffer
    rx_buffer = new uint8_t[buffer_size];

    return SUCCESS;
}

ssize_t NetworkReceiver::rx() {
    socklen_t addr_len = sizeof(struct sockaddr_in);

    return recvfrom(sockfd, (char*)rx_buffer, buffer_size, MSG_TRUNC,
                        (struct sockaddr*)&remote_addr, &addr_len);
}


AutoNetworkReceiver::AutoNetworkReceiver() {
    shm = NULL;
    inited = false;
}

AutoNetworkReceiver::~AutoNetworkReceiver() {
    MsgLogger logger("AutoNetworkReceiver", "destructor");

    if(shm) {
        if(SUCCESS != shm->detach()) {
            logger.log_message("failed to detach from shared memory");
        }

        delete shm;
    }
}

RetType AutoNetworkReceiver::init(VCM* vcm, uint16_t port, uint32_t multicast_addr, size_t rx_timeout, size_t buffer_size) {
    MsgLogger logger("AutoNetworkReceiver", "init");

    if(inited) {
        logger.log_message("already initialized");
        return FAILURE;
    }

    net_info_t* net = vcm->get_auto_net(port);
    if(NULL == net) {
        logger.log_message("no network device in auto configuration on this port");
        return FAILURE;
    }

    this->device_id = net->unique_id;

    shm = new NmShm();

    if(SUCCESS != shm->init(vcm->num_net_devices)) {
        logger.log_message("failed to initialize network manager shared memory");
        return FAILURE;
    }

    if(SUCCESS != shm->attach()) {
        logger.log_message("failed to attach to network manager shared memory");
        return FAILURE;
    }

    inited = true;

    return NetworkReceiver::init(port, multicast_addr, rx_timeout, buffer_size);
}

ssize_t AutoNetworkReceiver::rx() {
    socklen_t addr_len = sizeof(struct sockaddr_in);

    ssize_t read;
    read = recvfrom(sockfd, (char*)rx_buffer, buffer_size, MSG_TRUNC,
                        (struct sockaddr*)&remote_addr, &addr_len);

    if(-1 == read) {
        MsgLogger logger("AutoNetworkReceiver", "rx");
        logger.log_message("recvfrom failed");

        return -1;
    }

    if(FAILURE == shm->update_addr(device_id, &remote_addr)) {
        MsgLogger logger("AutoNetworkReceiver", "rx");
        logger.log_message("failed to update shared memory");
    }

    return read;
}

NetworkTransmitter::NetworkTransmitter() {
    mq = (mqd_t)-1;
    sockfd = -1;
    auto_config = false;
    inited = false;
    shm = NULL;
}

NetworkTransmitter::~NetworkTransmitter() {
    MsgLogger logger("NetworkTransmitter", "destructor");

    if((mqd_t)-1 != mq) {
        if(0 != mq_unlink(mqueue_name.c_str())) {
            logger.log_message("unable to close mqueue");
        }
    }

    if(-1 != sockfd) {
        if(0 != close(sockfd)) {
            logger.log_message("unable to close socket");
        }
    }

    if(shm) {
        if(SUCCESS != shm->detach()) {
            logger.log_message("failed to detach from shared memory");
        }

        delete shm;
    }
}

RetType NetworkTransmitter::init(uint32_t device_id, VCM* vcm, uint16_t port) {
    MsgLogger logger("NetworTransmitter", "init(device_id)");

    if(inited) {
        logger.log_message("already initialized");
        return FAILURE;
    }

    this->device_id = device_id;

    // mqueue name is /net[device_id]
    mqueue_name = "/net[";
    mqueue_name += std::to_string(device_id);
    mqueue_name += "]";

    // open the mqueue
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = NM_MAX_MSG_SIZE; // this limits how many bytes we can send
    attr.mq_curmsgs = 0;

    // open the mqueue
    // NOTE: we open it in blocking mode
    mq = mq_open(mqueue_name.c_str(), O_RDONLY|O_CREAT, 0644, &attr);
    if((mqd_t)-1 == mq) {
        logger.log_message("unable to open mqueue");
        return FAILURE;
    }

    // setup our socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { // ipv4, UDP
       logger.log_message("socket creation failed");
       return FAILURE;
    }

    // set REUSEPORT and REUSEADDR options
    // related release notes -> https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=c617f398edd4db2b8567a28e899a88f8f574798d
    int on = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        logger.log_message("failed to set socket reuseaddr option");
        return FAILURE;
    }

    on = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
        logger.log_message("failed to set socket reuseport option");
        return FAILURE;
    }

    // setup our address to send from
    // NOTE: if port is 0, the OS will pick a port when we bind
    struct sockaddr_in myaddr;
    memset(&myaddr, 0, sizeof(struct sockaddr_in));
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY); // use any interface we have available (likely just 1 ip)
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(port); // this is the port we send FROM

    int rc = bind(sockfd, (struct sockaddr*) &myaddr, sizeof(myaddr));
    if(rc) {
        logger.log_message("socket bind failed");
        return FAILURE;
    }

    shm = new NmShm();

    if(SUCCESS != shm->init(vcm->num_net_devices)) {
        logger.log_message("failed to initialized network manager shared memory");
        return FAILURE;
    }

    if(SUCCESS != shm->attach()) {
        logger.log_message("failed to attach to network manager shared memory");
        return FAILURE;
    }

    // get one address so we have a destination point, even if we're in auto mode
    // first call to 'get_addr' guarantees we get the lock
    if(FAILURE == shm->get_addr(device_id, &dst_addr)) {
        logger.log_message("failed to retrieve current address from shareed memory");
        return FAILURE;
    }

    if(port != 0) {
        if(NULL != vcm->get_auto_net(port)) {
            auto_config = true;
        }
    }

    return SUCCESS;
}

RetType NetworkTransmitter::init(std::string& device_name, VCM* vcm, uint16_t port) {
    MsgLogger logger("NetworkTransmitter", "init(device_name)");

    net_info_t* net = vcm->get_net(device_name);

    if(NULL == net) {
        logger.log_message("no network device with name: " + device_name);
        return FAILURE;
    }

    return init(net->unique_id, vcm, port);
}

RetType NetworkTransmitter::tx() {
    MsgLogger logger("NetworkTransmitter", "tx");

    ssize_t read = -1;
    read = mq_receive(mq, (char*)buffer, NM_MAX_MSG_SIZE, NULL);

    if(read != -1) {
        if(auto_config) {
            shm->get_addr(device_id, &dst_addr);

            if(0 == dst_addr.sin_addr.s_addr) {
                logger.log_message("receiver device has not yet sent a packet providing an"
                                    " address, failed to send UDP message");
                return FAILURE;
            }
        } // otherwise we have a hardcoded endpoint

        // transmit the packet
        ssize_t sent = -1;
        sent = sendto(sockfd, (char*)buffer, read, 0,
            (struct sockaddr*)&dst_addr, sizeof(dst_addr)); // send to whatever we last received from
        if(sent == -1) {
            logger.log_message("failed to send UDP message");
            return FAILURE;
        }

        return SUCCESS;
    }

    logger.log_message("failed to retrieve message from mqueue");
    return FAILURE;
}


NetworkInterface::NetworkInterface() {
    mq = (mqd_t)-1;
    inited = false;
}

NetworkInterface::~NetworkInterface() {
    MsgLogger logger("NetworkInterface", "destructor");

    if((mqd_t)-1 != mq) {
        if((mqd_t)-1 == mq_close(mq)) {
            logger.log_message("failed to close mqueue");
        }
    }
}

RetType NetworkInterface::init(uint32_t device_id) {
    MsgLogger logger("NetworkInterface", "init");

    if(inited) {
        logger.log_message("already initialized");
        return FAILURE;
    }

    mqueue_name = "/net[";
    mqueue_name += std::to_string(device_id);
    mqueue_name += "]";

    // turned non blocking on so if the queue is full it won't be logged (could be a potential issue if messages are being dropped)
    mq = mq_open(mqueue_name.c_str(), O_WRONLY|O_NONBLOCK); // TODO consider adding O_CREAT here, or actually should it fail if the network transmitter doesnt exist?
    if((mqd_t)-1 == mq) {
        logger.log_message("unable to open mqueue");
        return FAILURE;
    }

    inited = true;
    return SUCCESS;
}

RetType NetworkInterface::QueueUDPMessage(const uint8_t* msg, size_t size) {
    MsgLogger logger("NetworkInterface", "QueueUDPMessage");

    if(size > NM_MAX_MSG_SIZE) {
        logger.log_message("Message too large");
        return FAILURE;
    }

    if(0 > mq_send(mq, (const char*)msg, size, 0)) {
        logger.log_message("Failed to queue message");
        return FAILURE;
    }

    return SUCCESS;
}
