#ifndef NM_H
#define NM_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <mqueue.h>
#include <string>
#include "lib/vcm/vcm.h"
#include "common/types.h"
#include "lib/nm/NmShm.h"

#define NM_MAX_MSG_SIZE 2048

namespace nm {

    // receives packets over the network
    // listens on a port
    // if the port is used as an auto port for a network device, writes address to shared memory when it receives
    // NOTE: should only have one per port! or else the packets get split between the two
    class NetworkReceiver {
    public:
        // constructor
        NetworkReceiver();

        // destructor
        virtual ~NetworkReceiver();

        // initialize
        // listens on port 'port'
        // if multicast_addr is non-zero, joins the multicast group to listen for packets
        // if rx_timeout is > 0, receives timeout after that many millsec, otherwise it blocks indefinitely
        // stores received data in a buffer of size 'buffer_size'
        RetType init(uint16_t port, uint32_t multicast_addr = 0, size_t rx_timeout = 0, size_t buffer_size = 2048);

        // stores received data
        uint8_t* rx_buffer;

        // receive a packet
        // blocking if rx_timeout < 0
        // returns how many bytes were read, or -1 on error
        virtual ssize_t rx();

    private:
        bool inited;

    protected:
        size_t buffer_size;
        int sockfd;
        struct sockaddr_in remote_addr;
    };

    // receives packets over the network from a device with an auto configuration
    // whenever a packet is received, the source address is written to shared memory
    // so any network transmitter sending to this device can use that address to send to
    class AutoNetworkReceiver: public NetworkReceiver {
    public:
        // constructor
        AutoNetworkReceiver();

        // destructor
        ~AutoNetworkReceiver();

        // initialize
        // saves source address to network device with 'device_id' in shared memory
        // listens on port 'port'
        // if multicast_addr is non-zero, joins the multicast group to listen for packets
        // if rx_timeout is > 0, receives timeout after that many millsec, otherwise it blocks
        // stores received data in a buffer of size 'buffer_size'
        RetType init(vcm::VCM* vcm, uint16_t port, uint32_t multicast_addr = 0, size_t rx_timeout = 0, size_t buffer_size = 2048);

        // overrides base class 'rx'
        ssize_t rx();
    private:
        NmShm* shm;
        uint32_t device_id;
        bool inited;
    };

    // transmits packets over the network
    // reads messages from an mqueue with name /net[network_device_id]
    // NOTE: should only have one per device! doesn't break anything since they both read from the mqueue, but doesn't really make sense either
    class NetworkTransmitter {
    public:
        // constructor
        NetworkTransmitter();

        // destructor
        ~NetworkTransmitter();

        // sends to device 'device_name' from port 'port'
        // if no port is specified, lets the OS pick
        RetType init(std::string& device_name, vcm::VCM* vcm, uint16_t port = 0);
        RetType init(uint32_t device_id, vcm::VCM* vcm, uint16_t port = 0);

        // transmit any packets from the mqueue out to the network
        // NOTE: blocking operation, waits for a message in the mqueue if there is none
        RetType tx();

    private:
        mqd_t mq;
        std::string mqueue_name;

        uint32_t device_id;

        int sockfd;
        bool inited;
        uint8_t buffer[NM_MAX_MSG_SIZE];

        // shared memory object that holds destination addresses
        NmShm* shm;

        // if we're in an auto configuration and need to check for the destination address before sending
        bool auto_config;

        // destination address
        struct sockaddr_in dst_addr;
    };

    // gives an interface to request the network transmitter send a packet
    // can have many of these!
    // queues message up to mqueue with name /net[network_device_id]
    class NetworkInterface {
    public:
        // constructor
        NetworkInterface();

        // destructor
        ~NetworkInterface();

        // initialize interface
        RetType init(std::string& device_name, vcm::VCM* vcm);
        RetType init(uint32_t device_id);

        // queues a message to be sent by a corresponding NetworkTransmitter
        // puts a message in the mqueue /net[device_id]
        RetType QueueUDPMessage(const uint8_t* msg, size_t size);
    private:
        mqd_t mq;
        std::string mqueue_name;

        bool inited;
    };
}

#endif
