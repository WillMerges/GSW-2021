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

#define NM_MAX_MSG_SIZE 4096

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
        // places received data in 'buffer' of 'size' bytes
        // a receive attempt will timeout after 'rx_timeout' milliseconds
        // if 'rx_timeout' is set to -1 (or anything less than 0), all calls to receive will be blocking
        NetworkManager(uint16_t port, const char* name, uint8_t* buffer,
                    size_t size, ssize_t rx_timeout = -1);

        // destructor
        ~NetworkManager();

        RetType Open();
        RetType Close(); // returns fail if anything goes wrong

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

        uint8_t* rx_buffer;
        size_t rx_size;
        ssize_t rx_timeout;

        uint8_t tx_buffer[NM_MAX_MSG_SIZE];

        bool open;

        NmShm shm;
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
        RetType QueueUDPMessage(const char* msg, size_t size);
    private:
        bool open;
        std::string mqueue_name;
        mqd_t mq;
    };
}

#endif
