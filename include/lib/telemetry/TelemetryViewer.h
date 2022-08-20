/*******************************************************************************
* Name: TelemetryViewer.h
*
* Purpose: Top level read access to telemetry measurements
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
#ifndef TELVIEW_H
#define TELVIEW_H

#include <stdint.h>
#include "lib/telemetry/TelemetryShm.h"
#include "lib/vcm/vcm.h"
#include "common/types.h"

using namespace vcm;

// NOTE: signals should be caught when using this class in blocking mode
//       shared memory could be locked up if a read lock is held when the process is killed
//       to avoid this, catch the signal and wait until 'update' returns before exiting
//       since 'update' is a blocking call it will still block after a signal is caught so
//       the process cannot be released, to exit from 'update' call 'sighandler' in the signal
//       handler and 'update' will immediately return with
class TelemetryViewer {
public:
    // construct a telemetry viewer
    TelemetryViewer();

    // destructor
    ~TelemetryViewer();

    // initialize the telemetry viewer for vehicle specified by VCM
    // if 'shm' is not specified, creates a new TelemetryShm object
    // NOTE: if shm is not NULL, it should already be opened and initialized
    RetType init(std::shared_ptr<VCM> vcm, std::shared_ptr<TelemetryShm> shm = nullptr);

    // init using use the default VCM
    // if 'shm' is not specified, creates a new TelemetryShm object
    // NOTE: if shm is not NULL, it should already be opened
    RetType init(std::shared_ptr<TelemetryShm> shm = NULL);

    // remove all measurements currently viewable
    void remove_all();

    // add all measurements for the vehicle to be viewable
    RetType add_all();

    // add a single measurement to be viewable
    // NOTE: this will also add other measurements in the same packet
    RetType add(std::string& measurement);

    // add a single measurement to be viewable
    // NOTE: this will also add other measurements in the same packet
    RetType add(measurement_info_t* measurement);

    // add all the measurements in a single telemetry packet to be viewable
    RetType add(uint32_t packet_id);

    // update mode
    typedef enum {
        STANDARD_UPDATE, // simply refresh with the newest telemetry regardless if it's identical to the last update
        BLOCKING_UPDATE, // sleep the process until new telemetry (e.g. not in the last update) comes in and then return
        NONBLOCKING_UPDATE, // return BLOCKED if there is no new telemetry since the last update
    } update_mode_t;

    // set which update mode to use
    void set_update_mode(update_mode_t mode);

    // update the telemetry viewer with the most recent telemetry data
    // return FAILURE after 'timeout' milliseconds if the telemetry has not updated
    // if 'timeout' is 0, never times out and waits forever
    RetType update(uint32_t timeout = 0);

    // should be called by the processes signal handler, avoids locking shared memory on exit
    void sighandler();

    // set 'val' to the value of a telemetry measurement
    // converts raw telemetry data into usable types
    // returns FAILURE if type conversion is impossible
    RetType get_str(measurement_info_t* meas, std::string* val);
    RetType get_float(measurement_info_t* meas, float* val);
    RetType get_double(measurement_info_t* meas, double* val);
    RetType get_int(measurement_info_t* meas, int* val);
    RetType get_uint(measurement_info_t* meas, unsigned int* val);
    // place up to meas->size bytes into 'buffer'
    RetType get_raw(measurement_info_t* meas, uint8_t* buffer);

    // slightly slower functions that use strings as measurement parameters
    RetType get_str(std::string& meas, std::string* val);
    RetType get_float(std::string& meas, float* val);
    RetType get_double(std::string& meas, double* val);
    RetType get_int(std::string& meas, int* val);
    RetType get_uint(std::string& meas, unsigned int* val);
    RetType get_raw(std::string& meas, uint8_t* buffer);
    RetType get_raw(measurement_info_t* meas, uint8_t** buffer); // no copy

    // set 'data' to the most recently updated memory containing 'measurement'
    RetType latest_data(measurement_info_t* meas, uint8_t** data);

    // return true if the measurement was updated in the last call to 'update'
    bool updated(measurement_info_t* meas);

private:
    std::shared_ptr<TelemetryShm> shm;
    bool rm_shm = false;

    std::shared_ptr<VCM> vcm;
    bool rm_vcm = false;

    update_mode_t update_mode;
    bool check_all; // if we're tracking all measurements

    std::unique_ptr<unsigned int> packet_ids;
    size_t num_packets; // number of packets being tracked

    std::unique_ptr<std::unique_ptr<uint8_t[]>> packet_buffers;
    std::unique_ptr<size_t> packet_sizes;
};

#endif
