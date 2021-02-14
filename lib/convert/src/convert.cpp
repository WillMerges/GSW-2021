#include "lib/convert/convert.h"
#include <stdint.h>
#include <string.h>

using namespace convert;
using namespace vcm;

char result[MAX_CONVERSION_SIZE];

// TODO maybe have a fancy function with va_args, idk


// NOTE: may be a problem if unsigned and signed ints aren't the same length
//       but that would be weird to have them not the same length
RetType convert_str(VCM* vcm, measurement_info_t* measurement, const void* data, std::string* dst) {
    switch(measurement->type) {
        case INT_TYPE: {
            if(measurement->size > (sizeof(long int))) {
                return FAILURE; // TODO add logging
            }

            // assume unsigned and signed are the same size (potential problem)
            unsigned char val[sizeof(long int)]; // always assume it's the biggest (don't care about a few bytes)
            memset(val, 0, sizeof(long int));

            size_t addr = (size_t)measurement->addr;
            const unsigned char* buff = (const unsigned char*)data;

            if(vcm->recv_endianness != vcm->sys_endianness) {
                for(size_t i = 0; i < measurement->size; i++) {
                    val[sizeof(long int) - i - 1] = buff[addr + i];
                }
            } else {
                for(size_t i = 0; i < measurement->size; i++) {
                    val[i] = buff[addr + i];
                }
            }

            if(measurement->sign == SIGNED_TYPE) { // signed
                snprintf(result, MAX_CONVERSION_SIZE, "%li", *((int64_t*)val));
            } else { // unsigned
                snprintf(result, MAX_CONVERSION_SIZE, "%lu", *((uint64_t*)val));
            }

            }
            break;

        case FLOAT_TYPE: {
            if(measurement->size > (sizeof(double))) {
                return FAILURE; // TODO add logging
            }

            unsigned char val[sizeof(double)]; // always assume it's the biggest (don't care about a few bytes)
            memset(val, 0, sizeof(double));

            size_t addr = (size_t)measurement->addr;
            const unsigned char* buff = (const unsigned char*)data;

            if(vcm->recv_endianness != vcm->sys_endianness) {
                for(size_t i = 0; i < measurement->size; i++) {
                    val[sizeof(double) - i - 1] = buff[addr + i];
                }
            } else {
                for(size_t i = 0; i < measurement->size; i++) {
                    val[i] = buff[addr + i];
                }
            }

            // sign doesn't exist for floating point
            snprintf(result, MAX_CONVERSION_SIZE, "%f", *((double*)val));

            }
            break;

        case STRING_TYPE: {
            // -1 is for room for a null terminator
            if(measurement->size > MAX_CONVERSION_SIZE - 1) {
                return FAILURE; // TODO add logging
            }
            size_t addr = (size_t)measurement->addr;
            const unsigned char* buff = (const unsigned char*)data;
            memcpy(result, buff + addr, measurement->size);
            result[measurement->size] = '\0'; // null terminate just in case

            }
            break;

        default:
            // TODO maybe display as hex?
            return FAILURE;
    }

    *dst = result;
    return SUCCESS;
}
