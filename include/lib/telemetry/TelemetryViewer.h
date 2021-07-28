/*******************************************************************************
* Name: TelemetryViewer.h
*
* Purpose: Top level access to telemetry measurements
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
#ifndef TELVIEW_H
#define TELVIEW_H

#include "lib/telemetry/TelemetryShm.h"
#include "lib/vcm/vcm.h"
#include "common/types.h"

using namespace vcm;

class TelemetryViewer {
public:
    // construct a telemetry viewer
    TelemetryViewer();

    // destructor
    ~TelemetryViewer();

    // initialize the telemetry viewer for vehicle specified by VCM
    RetType init(VCM* vcm);

    // init using use the default VCM
    RetType init();

    // remove all measurements currently viewable
    void remove_all();

    // add all measurements for the vehicle to be viewable
    RetType add_all();

    // add a single measurement to be viewable
    // NOTE: this will also add other measurements in the same packet
    RetType add(std::string measurement);

    // add all the measurements in a single telemetry packet to be viewable
    RetType add(unsigned int packet_id);

    // update mode
    typedef enum {
        STANDARD_UPDATE, // simply refresh with the newest telemetry regardless if it's identical to the last update
        BLOCKING_UPDATE, // sleep the process until new telemetry (e.g. not in the last update) comes in and then return
        NONBLOCKING_UPDATE, // return BLOCKED if there is no new telemetry since the last update
    } update_mode_t;

    // set which update mode to use
    void set_update_mode(update_mode_t mode);

    // update the telemetry viewer with the most recent telemetry data
    RetType update();

    // set 'val' to the value of a telemetry measurement
    // converts raw telemetry data into usable types
    // returns FAILURE if type conversion is impossible
    RetType get_str(std::string measurement, std::string* val);
    RetType get_float(std::string measurement, float* val);
    RetType get_double(std::string measurement, double* val);
    RetType get_int(std::string measurement, int* val);
    RetType get_uint(std::string measurement, unsigned int* val);
    // place up to 'size' bytes into 'data'
    // returns the actual size of the measurement, or -1 if the measurement is larger than 'size'
    ssize_t get_raw(std::string measurement, uint8_t* data, size_t size);

private:
    TelemetryShm shm;
    VCM* vcm;

    update_mode_t update_mode;
    bool check_all; // if we're tracking all measurements

    unsigned int* packet_ids;
    size_t num_packets; // number of packets being tracked

    uint8_t** packet_buffers;
    size_t* packet_sizes;


    // set 'data' to the most recently updated memory containing 'measurement'
    RetType latest_data(measurement_info_t* meas, uint8_t** data);
};

#endif
