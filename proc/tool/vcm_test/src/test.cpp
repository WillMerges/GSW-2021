#include <iostream>
#include <string>
#include "lib/vcm/vcm.h"

using namespace vcm;

int main() {
    VCM vcm;

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
            std::cout << '\n';
        }

    }
}
