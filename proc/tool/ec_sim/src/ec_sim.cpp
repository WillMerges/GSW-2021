#include "lib/ec/ec.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// solenoid states
#define NUM_SOLENOIDS 4
// solenoids start at control number 100, e.g. solenoid 0 is 100, 1 is 101 ...
uint16_t solenoid_control_start = 100; // this is an arbitrary number
uint8_t solenoids[NUM_SOLENOIDS];

// igniter states
#define NUM_IGNITERS 1
uint16_t igniter_control_start = 200; // this is an arbitrary number
uint8_t igniters[NUM_IGNITERS];

// IPv4 address of the ground station
const char* gs_ip = "127.0.0.1";

// greatest sequence acknowledged
uint32_t seq_num = 0;

// port numbers of telemetry packets sent TO the ground station
#define SOLENOID_PORT 8081
#define IGNITER_PORT  8082
#define CMD_ACK_PORT  8083

// port to bind the engine controller to
#define EC_PORT 8080

// src and dst address structures
struct sockaddr_in dst_addr;
struct sockaddr_in src_addr;

// socket descriptor
int sockfd;

// send an acknowledgement command for the current greatest sequence number
void ack_cmd() {
    // send to wherever we got the packet from, but to the predefined port
    dst_addr.sin_port = htons(CMD_ACK_PORT);

    // should really check return, but this is just a sim
    sendto(sockfd, (void*)&seq_num, sizeof(seq_num), 0,
                (struct sockaddr*)&dst_addr, sizeof(dst_addr));
}

// send a telemetry packet of the solenoid states
void send_solenoid_state() {
        // send to wherever we got the packet from, but to the predefined port
        dst_addr.sin_port = htons(SOLENOID_PORT);

        // should really check return, but this is just a sim
        sendto(sockfd, (void*)solenoids, sizeof(uint8_t) * NUM_SOLENOIDS, 0,
                    (struct sockaddr*)&dst_addr, sizeof(dst_addr));
}

// send a telemetry packet of the igniter states
void send_igniter_state() {
        // send to wherever we got the packet from, but to the predefined port
        dst_addr.sin_port = htons(IGNITER_PORT);

        // should really check return, but this is just a sim
        sendto(sockfd, (void*)igniters, sizeof(uint8_t) * NUM_IGNITERS, 0,
                    (struct sockaddr*)&dst_addr, sizeof(dst_addr));
}

// print the state of the engine controller
void print_state() {
    // clear the screen
    printf("\033[2J");

    // print solenoid states
    for(int i = 0; i < NUM_SOLENOIDS; i++) {
        printf("solenoid %d: 0x%02x\n", i, solenoids[i]);
    }

    // print igniter states
    for(int i = 0; i < NUM_IGNITERS; i++) {
        printf("igniter %d: 0x%02x\n", i, igniters[i]);
    }

    // print highest sequence number
    printf("sequence: %u\n", seq_num);
}

int main() {
    // set up our address
    src_addr.sin_family = AF_INET; // IPv4
    src_addr.sin_addr.s_addr = htonl(INADDR_ANY); // use any interface
    src_addr.sin_port = htons(EC_PORT);

    // create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(-1 == sockfd) {
        printf("failed to create UDP socket\n");
        return -1;
    }

    // set socket options so we can use the same port
    // allows us to reuse ports with different procs (e.g. decom and this sim)
    int on = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        printf("socket option REUSEADDR failed\n");
        return -1;
    }

    on = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
        printf("socket option REUSEPORt failed\n");
        return -1;
    }

    // bind the socket to our address
    if(-1 == bind(sockfd, (struct sockaddr*)&src_addr, sizeof(src_addr))) {
        printf("failed to bind UDP socket\n");
        return -1;
    }

    // send out all of our telemetry first
    ack_cmd();
    send_solenoid_state();
    send_igniter_state();

    // listen for command packets
    uint8_t recv_buffer[sizeof(ec_command_t)];

    ssize_t len;
    socklen_t addr_len = sizeof(dst_addr);
    while(1) {
        print_state();

        len = recvfrom(sockfd, (void*)recv_buffer, sizeof(ec_command_t),
                       MSG_TRUNC, (struct sockaddr*)&dst_addr, &addr_len);

        // either recvfrom failed or we got a non-command packet
        if(len == -1 || len > (ssize_t)(sizeof(ec_command_t))) {
            continue;
        }

        // look at the command packet
        ec_command_t* cmd = (ec_command_t*)recv_buffer;
        if(cmd->seq_num <= seq_num) {
            // we should have already parsed this command, or it got dropped
            continue;
        }

        // otherwise we have a new command to carry out
        seq_num = cmd->seq_num;
        ack_cmd();

        if(cmd->control >= solenoid_control_start &&
                cmd->control < solenoid_control_start + NUM_SOLENOIDS) {
            // this is a command for one of the solenoids
            uint16_t solenoid_num = cmd->control % solenoid_control_start;
            solenoids[solenoid_num] = cmd->state;

            send_solenoid_state();
        }

        if(cmd->control >= igniter_control_start &&
                cmd->control < igniter_control_start + NUM_IGNITERS) {
            // this is a command for one of the igniters 
            uint16_t igniter_num = cmd->control % igniter_control_start;
            igniters[igniter_num] = cmd->state;

            send_igniter_state();
        }
    }
}
