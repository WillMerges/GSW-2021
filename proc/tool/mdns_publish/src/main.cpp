/********************************************************************
*  Name: main.cpp
*
*  Purpose: Publishes all currently used IPv4 addresses over mDNS.
*           Requires avahi-daemon to be installed and running.
*           Starts a process for each address to publish, writes out PIDs
*           to a given file.
*
*  Usage: ./publish_mdns [name] [PID_file]
*          Starts processes that advertises 'name' over all interfaces,
*          appends PIDs of started processes out to 'pid_file'
*          For standard usage 'name' should be something like myPC.local
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>

// write out PIDs of mDNS publishing processes
#define PID_FILE "mdns_pids"

int main(int argc, char* argv[]) {
    if(argc < 3) {
        printf("Usage: publish_mds [name] [PID_file]\n");
        return -1;
    }

    std::string cmd = "avahi-publish -R -a ";
    cmd += argv[1];
    cmd += " ";

    struct ifaddrs *ifap = NULL;
    getifaddrs(&ifap);

    struct sockaddr_in* ifa_addr = NULL;
    struct sockaddr_in* ifa_netmask = NULL;
    while(ifap) {
        // printf("name: %s\n", ifap->ifa_name);

        // check address
        ifa_addr = (struct sockaddr_in*)(ifap->ifa_addr);

        if(!ifa_addr) {
            // no address
            ifap = ifap->ifa_next;
            continue;
        }

        if(ifa_addr->sin_family != AF_INET) {
            // not an IPv4 address
            ifap = ifap->ifa_next;
            continue;
        }

        // check subnet
        ifa_netmask = (struct sockaddr_in*)(ifap->ifa_netmask);

        if(!ifa_netmask) {
            // no address
            ifap = ifap->ifa_next;
            continue;
        }

        if(ifa_netmask->sin_family != AF_INET) {
            // subnet is not IPv4
            ifap = ifap->ifa_next;
            continue;
        }

        if(ifap->ifa_flags & IFF_UP &&
           !(ifap->ifa_flags & IFF_LOOPBACK) &&
           !(ifap->ifa_flags & IFF_NOARP) &&
           ifap->ifa_flags & IFF_RUNNING) {
               // publish this address
               printf("publishing name for %s on interface %s\n", inet_ntoa(ifa_addr->sin_addr), ifap->ifa_name);
               std::string c = cmd + inet_ntoa(ifa_addr->sin_addr);
               c += " > /dev/null & echo $! >> ";
               c += argv[2];
               system(c.c_str());
        }

        ifap = ifap->ifa_next;
    }
}
