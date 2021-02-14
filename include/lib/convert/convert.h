// conversion library

#ifndef CONVERT_H
#define CONVERT_H

#include "common/types.h"
#include "lib/vcm/vcm.h"
#include <string>

// can't convert any measurement larger than this
#define MAX_CONVERSION_SIZE 256

namespace convert {
    RetType convert_str(vcm::VCM* vcm, vcm::measurement_info_t* measurement, void* data, std::string* dst);
}

#endif
