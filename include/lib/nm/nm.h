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
    // checks an mqueue of name /'name' for messages to send over UDP
    // checks the UDP socket with 'port' for incoming messages and writes them
    // to passed in 'buffer' of 'size' bytes
    // receiving can be blocking or non-blocking depending on how 'rx_timeout' is set

    // likely use is for receiving telemetry packets, should have one NetworkManager
    // per telemetry packet
    class NetworkManager {
    public:

        // construct a network manager named 'name'
        // this name must match any associated network interfaces
        // binds to 'port' (e.g. received packets dst = port, sent packets src = port)
        // ports in host order
        // places received data in 'buffer' of 'size' bytes
        // if buffer is NULL or size is 0, calling 'Receive' discards the packet
        // a receive attempt will timeout after 'rx_timeout' milliseconds
        // if 'dev_port' is set, sets the port for the control device, packets from
        // with this src port will be used as the address to send packets to when 'Send' is called
        // if 'rx_timeout' is set to -1 (or anything less than 0), all calls to receive will be blocking
        // if a multicast address other than 0 is specified, the network manager will listen on that group
        // the multicast address is expected in network order
        NetworkManager(uint16_t port, const char* name, uint8_t* buffer,
                    size_t size, uint16_t dev_port = 0, uint32_t multicast_addr = 0, ssize_t rx_timeout = -1);

        // destructor
        ~NetworkManager();

        // return FAILURE on failure
        RetType Open();
        RetType Close();

        // send any outgoing messages from the mqueue, return FAILURE on error
        RetType Send();

        // receive any messages and write them to 'buffer', return FAILURE on error.
        // the number of bytes received is written to 'buffer' up to 'size' bytes.
        // the total number of bytes received is placed in 'received',
        // even if it would be larger than the buffer size.
        RetType Receive(size_t* received);


        // TODO consider adding priority to sending some messages (like deployment)
    private:
        mqd_t mq;
        std::string mqueue_name;

        struct sockaddr_in device_addr; // address of the device we're listening to (filled in by recvfrom)
        int sockfd;

        uint16_t port;
        uint16_t dev_port;

        uint8_t* rx_buffer;
        size_t rx_size;
        ssize_t rx_timeout;

        uint8_t tx_buffer[NM_MAX_MSG_SIZE + 1];

        bool open;

        NmShm shm;

        uint32_t multicast_addr;
    };

    // allows a process to queue a message to send
    // can have many instances
    // sends a message to an associated NetworkManager's mqueue
    class NetworkInterface {
    public:
        NetworkInterface(const char* name);
        ~NetworkInterface();
        RetType Open();
        RetType Close();

        // Queues a UDP process to be sent by a network manager of name 'name'
        // Message will be sent over network when NetworkManager calls 'Send'
        // and reads this message from the mqueue
        RetType QueueUDPMessage(const char* msg, size_t size);
    private:
        bool open;
        std::string mqueue_name;
        mqd_t mq;
    };
}

#endif
