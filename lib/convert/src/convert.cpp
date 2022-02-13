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
            val[measurement->size - i - 1] = data[i];
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
            val[measurement->size - i - 1] = data[i];
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

    if(measurement->size > sizeof(uint32_t)) {
        logger.log_message("measurement too large to fit into integer");
        return FAILURE;
    }

    uint8_t val[sizeof(uint32_t)];
    memset(val, 0, sizeof(uint32_t));
    // size_t addr = (size_t)measurement->addr;
    // const uint8_t* buff = (const uint8_t*)data;

    if(vcm->recv_endianness != vcm->sys_endianness) {
        for(size_t i = 0; i < measurement->size; i++) {
            val[measurement->size - i - 1] = data[i];
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

    if(measurement->type != INT_TYPE || measurement->sign != SIGNED_TYPE) {
        logger.log_message("Measurement must be a signed integer!");
        return FAILURE;
    }

    if(measurement->size > sizeof(int32_t)) {
        logger.log_message("measurement too large to fit into integer");
        return FAILURE;
    }

    uint8_t val[sizeof(int32_t)];
    memset(val, 0, sizeof(int32_t));
    // size_t addr = (size_t)measurement->addr;
    // const uint8_t* buff = (const uint8_t*)data;

    uint8_t sign_byte;
    if(vcm->recv_endianness == GSW_BIG_ENDIAN) {
        // sign byte is first
        sign_byte = data[0];
    } else {
        // sign byte is last
        sign_byte = data[measurement->size - 1];
    }

    if(vcm->recv_endianness != vcm->sys_endianness) {
        for(size_t i = 0; i < measurement->size; i++) {
            val[measurement->size - i - 1] = data[i];
        }
    } else {
        for(size_t i = 0; i < measurement->size; i++) {
            val[i] = data[i];
        }
    }

    *dst = *((int32_t*)val);

    if(sign_byte & (1 << 7)) {
        *dst *= -1;
    }

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
            if(measurement->sign == SIGNED_TYPE) { // signed
                int32_t val;
                RetType ret = convert_to(vcm, measurement, data, &val);

                if(ret != SUCCESS) {
                    return ret;
                }

                snprintf(result, MAX_CONVERSION_SIZE, "%i", val);
            } else { // unsigned
                uint32_t val;
                RetType ret = convert_to(vcm, measurement, data, &val);

                if(ret != SUCCESS) {
                    return ret;
                }

                snprintf(result, MAX_CONVERSION_SIZE, "%u", val);
            }

            }
            break;

        // encompasses 4-byte floats and 8-byte doubles
        case FLOAT_TYPE: {
            if(measurement->size == sizeof(float)) {
                float val;
                RetType ret = convert_to(vcm, measurement, data, &val);

                if(ret != SUCCESS) {
                    return ret;
                }

                snprintf(result, MAX_CONVERSION_SIZE, "%f", val);
            } else if(measurement->size == sizeof(double)) {
                double val;
                RetType ret = convert_to(vcm, measurement, data, &val);

                if(ret != SUCCESS) {
                    return ret;
                }

                snprintf(result, MAX_CONVERSION_SIZE, "%f", val);
            } else {
                logger.log_message("size of float type measurement does not match float or double");
                return FAILURE;
            }

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
