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
    // convert telemetry measurements to C data types
    RetType convert_to(vcm::VCM* vcm, vcm::measurement_info_t* measurement,
                                        const uint8_t* data, std::string* dst);
    RetType convert_to(vcm::VCM* vcm, vcm::measurement_info_t* measurement,
                                        const uint8_t* data, uint32_t* dst);
    RetType convert_to(vcm::VCM* vcm, vcm::measurement_info_t* measurement,
                                        const uint8_t* data, int32_t* dst);
    RetType convert_to(vcm::VCM* vcm, vcm::measurement_info_t* measurement,
                                        const uint8_t* data, float* dst);
    RetType convert_to(vcm::VCM* vcm, vcm::measurement_info_t* measurement,
                                        const uint8_t* data, double* dst);

    // convert from C data type back to raw telemetry
    // NOTE: all assume 'output' is at least as large as the measurement size
    RetType convert_from(vcm::VCM* vcm, vcm::measurement_info_t* measurement,
                            uint8_t* output, std::string& val);
    RetType convert_from(vcm::VCM* vcm, vcm::measurement_info_t* measurement,
                            uint8_t* output, uint32_t val);
    RetType convert_from(vcm::VCM* vcm, vcm::measurement_info_t* measurement,
                            uint8_t* output, int32_t val);
    RetType convert_from(vcm::VCM* vcm, vcm::measurement_info_t* measurement,
                            uint8_t* output, float val);
    RetType convert_from(vcm::VCM* vcm, vcm::measurement_info_t* measurement,
                            uint8_t* output, double& val);

}

#endif
