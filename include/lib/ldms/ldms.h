/********************************************************************
*  Name: ldms.h
*
*  Purpose: Header for launch data management system source.
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/

#ifndef LDMS_H
#define LDMS_H

#include <string>
#include <stdint.h>

namespace ldms {

    typedef struct {
        void* addr;
        size_t size;
    } measurement_info_t;

    class LDMS {
    public:
        LDMS(); // uses default config file
        LDMS(std::string config_file);
        measurement_info_t get_info(std::string measurement); // get the info of a measurement
    };

}

#endif
