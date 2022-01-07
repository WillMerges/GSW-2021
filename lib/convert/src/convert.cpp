#include "lib/convert/convert.h"
#include "lib/dls/dls.h"
#include <stdint.h>
#include <string.h>

using namespace vcm;
using namespace dls;

char result[MAX_CONVERSION_SIZE];

// TODO maybe have a fancy function with va_args, idk


RetType convert::convert_to(vcm::VCM* vcm, vcm::measurement_info_t* measurement, const uint8_t* data, double* dst) {
    MsgLogger logger("CONVERT", "convert_to");

    if(measurement->type != FLOAT_TYPE) {
        logger.log_message("Measurement must be a float type!");
        return FAILURE;
    }

    if(measurement->size != sizeof(double)) {
        logger.log_message("measurement is not the size of a double!");
        return FAILURE;
    }

    uint8_t val[sizeof(double)];
    // size_t addr = (size_t)measurement->addr;
    // const uint8_t* buff = (const uint8_t*)data;

    if(vcm->recv_endianness != vcm->sys_endianness) {
        for(size_t i = 0; i < measurement->size; i++) {
            val[sizeof(uint32_t) - i - 1] = data[i];
        }
    } else {
        for(size_t i = 0; i < measurement->size; i++) {
            val[i] = data[i];
        }
    }

    *dst = *((double*)val);
    return SUCCESS;
}

RetType convert::convert_to(vcm::VCM* vcm, vcm::measurement_info_t* measurement, const uint8_t* data, float* dst) {
    MsgLogger logger("CONVERT", "convert_to");

    if(measurement->type != FLOAT_TYPE) {
        logger.log_message("Measurement must be a float type!");
        return FAILURE;
    }

    if(measurement->size != sizeof(float)) {
        logger.log_message("measurement is not the size of a float!");
        return FAILURE;
    }

    uint8_t val[sizeof(float)];
    // size_t addr = (size_t)measurement->addr;
    // const uint8_t* buff = (const uint8_t*)data;

    if(vcm->recv_endianness != vcm->sys_endianness) {
        for(size_t i = 0; i < measurement->size; i++) {
            val[sizeof(uint32_t) - i - 1] = data[i];
        }
    } else {
        for(size_t i = 0; i < measurement->size; i++) {
            val[i] = data[i];
        }
    }

    *dst = *((float*)val);
    return SUCCESS;
}

RetType convert::convert_to(vcm::VCM* vcm, vcm::measurement_info_t* measurement, const uint8_t* data, uint32_t* dst) {
    MsgLogger logger("CONVERT", "convert_to");

    if(measurement->type != INT_TYPE || measurement->sign != UNSIGNED_TYPE) {
        logger.log_message("Measurement must be an unsigned integer!");
        return FAILURE;
    }

    if(measurement->size > sizeof(int32_t)) {
        logger.log_message("measurement too large to fit into integer");
        return FAILURE;
    }

    uint8_t val[sizeof(uint32_t)];
    // size_t addr = (size_t)measurement->addr;
    // const uint8_t* buff = (const uint8_t*)data;

    if(vcm->recv_endianness != vcm->sys_endianness) {
        for(size_t i = 0; i < measurement->size; i++) {
            val[sizeof(uint32_t) - i - 1] = data[i];
        }
    } else {
        for(size_t i = 0; i < measurement->size; i++) {
            val[i] = data[i];
        }
    }

    *dst = *((uint32_t*)val);
    return SUCCESS;
}

RetType convert::convert_to(vcm::VCM* vcm, vcm::measurement_info_t* measurement, const uint8_t* data, int32_t* dst) {
    MsgLogger logger("CONVERT", "convert_to");

    if(measurement->type != INT_TYPE || measurement->sign != UNSIGNED_TYPE) {
        logger.log_message("Measurement must be an unsigned integer!");
        return FAILURE;
    }

    if(measurement->size > sizeof(int32_t)) {
        logger.log_message("measurement too large to fit into integer");
        return FAILURE;
    }

    uint8_t val[sizeof(int32_t)];
    // size_t addr = (size_t)measurement->addr;
    // const uint8_t* buff = (const uint8_t*)data;

    if(vcm->recv_endianness != vcm->sys_endianness) {
        for(size_t i = 0; i < measurement->size; i++) {
            val[sizeof(int32_t) - i - 1] = data[i];
        }
    } else {
        for(size_t i = 0; i < measurement->size; i++) {
            val[i] = data[i];
        }
    }

    *dst = *((int32_t*)val);
    return SUCCESS;
}


// NOTE: may be a problem if unsigned and signed ints aren't the same length
//       but that would be weird to have them not the same length
// TODO this function could use some cleaning, uses malloc when it doesnt need to, some unnecessary code from it being refactored so many times
// TODO now that other convert functions exist, maybe call them and then just convert to a string (sprintf)
RetType convert::convert_to(VCM* vcm, measurement_info_t* measurement, const uint8_t* data, std::string* dst) {
    MsgLogger logger("CONVERT", "convert_to");

    switch(measurement->type) {
        // all integer types, will try to put it a standard integer type of the same size or larger than the measurement size
        // distinguishes between signed and unsigned
        case INT_TYPE: {
            if(measurement->size > (sizeof(long int))) {
                logger.log_message("Measurement size too great to convert to integer");
                return FAILURE;
            }

            // assume unsigned and signed are the same size (potential problem)
            unsigned char val[sizeof(long int)]; // always assume it's the biggest (don't care about a few bytes)
            memset(val, 0, sizeof(long int));

            // size_t addr = (size_t)measurement->addr;
            // const unsigned char* buff = (const unsigned char*)data;

            if(vcm->recv_endianness != vcm->sys_endianness) {
                for(size_t i = 0; i < measurement->size; i++) {
                    val[sizeof(long int) - i - 1] = data[i];
                }
            } else {
                for(size_t i = 0; i < measurement->size; i++) {
                    val[i] = data[i];
                }
            }

            if(measurement->sign == SIGNED_TYPE) { // signed
                snprintf(result, MAX_CONVERSION_SIZE, "%li", *((int64_t*)val));
            } else { // unsigned
                snprintf(result, MAX_CONVERSION_SIZE, "%lu", *((uint64_t*)val));
            }

            }
            break;

        // encompasses 4-byte floats and 8-byte doubles
        case FLOAT_TYPE: {
            size_t convert_size = 0;
            if(measurement->size == sizeof(float)) {
                convert_size = sizeof(float);
            } else if(measurement->size == sizeof(double)) {
                convert_size = sizeof(double);
            }

            if(convert_size == 0) {
                logger.log_message("Unable to convert floating type to float or double");
                return FAILURE;
            }

            unsigned char* val = (unsigned char*)malloc(convert_size);
            memset(val, 0, convert_size);

            // size_t addr = (size_t)measurement->addr;
            // const unsigned char* buff = (const unsigned char*)data;

            if(vcm->recv_endianness != vcm->sys_endianness) {
                for(size_t i = 0; i < measurement->size; i++) {
                    val[convert_size - i - 1] = data[i];
                }
            } else {
                for(size_t i = 0; i < measurement->size; i++) {
                    val[i] = data[i];
                }
            }

            // sign doesn't exist for floating point
            snprintf(result, MAX_CONVERSION_SIZE, "%f", *((float*)val));

            free(val);
            }
            break;

        case STRING_TYPE: {
            // -1 is for room for a null terminator
            if(measurement->size > MAX_CONVERSION_SIZE - 1) {
                logger.log_message("string too large to parse");
                return FAILURE;
            }
            // size_t addr = (size_t)measurement->addr;
            // const unsigned char* buff = (const unsigned char*)data;
            memcpy(result, data, measurement->size);
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


// write out each character in 'str' to 'output'
// NOTE: measurement size must be at least as large as the string
// ignore endianness for strings
RetType convert::convert_from(vcm::VCM*, vcm::measurement_info_t* measurement, uint8_t* output, std::string& val) {
    MsgLogger logger("CONVERT", "convert_from(str)");

    if(measurement->size < val.size() + 1) {
        logger.log_message("measurement too small to hold string");
        return FAILURE;
    }

    // endianness doesn't matter for strings
    size_t i = 0;
    for(; i < measurement->size; i++) {
        output[i] = (uint8_t)(val[i]);
    }

    // add null-terminator
    output[i] = '\0';

    return SUCCESS;
}

RetType convert::convert_from(vcm::VCM* vcm, vcm::measurement_info_t* measurement, uint8_t* output, uint32_t val) {
    MsgLogger logger("CONVERT", "convert_from(uint32)");

    if(measurement->size < sizeof(uint32_t)) {
        logger.log_message("measurement too small to hold uint32");
        return FAILURE;
    }

    uint8_t* valb = (uint8_t*)&val;

    if(vcm->sys_endianness != vcm->recv_endianness) {
        // pad the beginning with zeros
        memset(output, 0, measurement->size - sizeof(uint32_t));

        size_t i = 0;
        for(; i < sizeof(uint32_t); i++) {
            output[measurement->size - sizeof(uint32_t) + i] = valb[sizeof(uint32_t) - i - 1];
        }

    } else {
        size_t i = 0;
        for(; i < sizeof(uint32_t); i++) {
            output[i] = valb[i];
        }

        // zero the rest of the output
        memset(output + i, 0, measurement->size - sizeof(uint32_t));
    }

    return SUCCESS;
}

RetType convert::convert_from(vcm::VCM* vcm, vcm::measurement_info_t* measurement, uint8_t* output, int32_t val) {
    MsgLogger logger("CONVERT", "convert_from(int32)");

    if(measurement->size < sizeof(int32_t)) {
        logger.log_message("measurement too small to hold int32");
        return FAILURE;
    }

    uint8_t* valb = (uint8_t*)&val;

    if(vcm->sys_endianness != vcm->recv_endianness) {
        // pad the beginning with zeros
        memset(output, 0, measurement->size - sizeof(int32_t));

        size_t i = 0;
        for(; i < sizeof(int32_t); i++) {
            output[measurement->size - sizeof(int32_t) + i] = valb[sizeof(int32_t) - i - 1];
        }

    } else {
        size_t i = 0;
        for(; i < sizeof(int32_t); i++) {
            output[i] = valb[i];
        }

        // zero the rest of the output
        memset(output + i, 0, measurement->size - sizeof(int32_t));
    }

    return SUCCESS;
}

RetType convert::convert_from(vcm::VCM* vcm, vcm::measurement_info_t* measurement, uint8_t* output, float val) {
    MsgLogger logger("CONVERT", "convert_from(float)");

    if(measurement->size != sizeof(float)) {
        logger.log_message("measurement not the same size as float");
        return FAILURE;
    }

    uint8_t* valb = (uint8_t*)&val;

    if(vcm->sys_endianness != vcm->recv_endianness) {
        size_t i = 0;
        for(; i < sizeof(float); i++) {
            output[i] = valb[sizeof(float) - i - 1];
        }

    } else {
        size_t i = 0;
        for(; i < sizeof(float); i++) {
            output[i] = valb[i];
        }
    }

    return SUCCESS;
}

RetType convert::convert_from(vcm::VCM* vcm, vcm::measurement_info_t* measurement, uint8_t* output, double& val) {
    MsgLogger logger("CONVERT", "convert_from(double)");

    if(measurement->size != sizeof(double)) {
        logger.log_message("measurement not the same size as double");
        return FAILURE;
    }

    uint8_t* valb = (uint8_t*)&val;

    if(vcm->sys_endianness != vcm->recv_endianness) {
        size_t i = 0;
        for(; i < sizeof(double); i++) {
            output[i] = valb[sizeof(double) - i - 1];
        }

    } else {
        size_t i = 0;
        for(; i < sizeof(double); i++) {
            output[i] = valb[i];
        }
    }

    return SUCCESS;
}
