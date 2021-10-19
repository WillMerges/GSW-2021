#include <iostream>
#include <string>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "lib/vcm/vcm.h"

using namespace vcm;

int main() {
    VCM vcm;

    if(vcm.init() == FAILURE) {
        std::cout << "Failed to initialize VCM\n";
        exit(-1);
    }

    std::cout << "device id: " << vcm.device << '\n';

    std::cout << "receiver endianness: ";
    if(vcm.recv_endianness == GSW_LITTLE_ENDIAN) {
        std::cout << "little endian\n";
    } else if(vcm.recv_endianness == GSW_BIG_ENDIAN) {
        std::cout << "big endian\n";
    }

    std::cout << "system endianness: ";
    if(vcm.sys_endianness == GSW_LITTLE_ENDIAN) {
        std::cout << "little endian\n";
    } else if(vcm.sys_endianness == GSW_BIG_ENDIAN) {
        std::cout << "big endian\n";
    }

    std::cout << "destination port: " << vcm.port << "\n";

    if(vcm.multicast_addr != 0) {
        struct in_addr addr;
        memset(&addr, 0, sizeof(addr));
        addr.s_addr = vcm.multicast_addr;
        std::cout << "multicast address: " << inet_ntoa(addr) << "\n";
    }

    std::cout << "\npackets: \n";
    int i = 0;
    for(packet_info_t* packet : vcm.packets) {
        std::cout << i++ << ": ";
        std::cout << "size: " << packet->size << " ";
        std::cout << "port: " << packet->port << "\n";
    }

    std::cout << '\n';

    for (auto& it : vcm.measurements) {
        std::cout << it << " ";

        measurement_info_t* info = vcm.get_info(it);
        if(info != NULL) {
            std::cout << info->size << " ";
            switch(info->type) {
                case UNDEFINED_TYPE:
                    std::cout << "undefined";
                    break;
                case INT_TYPE:
                    std::cout << "int";
                    break;
                case FLOAT_TYPE:
                    std::cout << "float";
                    break;
                case STRING_TYPE:
                    std::cout << "string";
                    break;
            }
            std::cout << "  [ ";
            for(location_info_t loc : info->locations) {
                std::cout << loc.packet_index << ":" << loc.offset << " ";
            }
            std::cout << "]";

            std::cout << '\n';
        }

    }
}
