// conversion library

#ifndef CONVERT_H
#define CONVERT_H

#include "common/types.h"
#include "lib/vcm/vcm.h"
#include <stdint.h>
#include <string>

// can't convert any measurement larger than this
#define MAX_CONVERSION_SIZE 256 // bytes

namespace convert {
    RetType convert_str(vcm::VCM* vcm, vcm::measurement_info_t* measurement, const void* data, std::string* dst);
    RetType convert_uint(vcm::VCM* vcm, vcm::measurement_info_t* measurement, const void* data, uint32_t* dst);
    RetType convert_int(vcm::VCM* vcm, vcm::measurement_info_t* measurement, const void* data, int32_t* dst);
    RetType convert_float(vcm::VCM* vcm, vcm::measurement_info_t* measurement, const void* data, float* dst);
    RetType convert_double(vcm::VCM* vcm, vcm::measurement_info_t* measurement, const void* data, double* dst);
}

#endif
