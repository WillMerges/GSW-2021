/*******************************************************************************
* Name: TelemetryViewer.h
*
* Purpose: Top level write access to telemetry measurements
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
#ifndef TELWRITE_H
#define TELWRITE_H

#include <stdint.h>
#include <stdlib.h>
#include <string>

#include "lib/telemetry/TelemetryShm.h"
#include "lib/dls/dls.h"
#include "lib/vcm/vcm.h"
#include "common/types.h"


using namespace vcm;
using namespace dls;

class TelemetryWriter {
public:
    // construct a telemetry writer
    TelemetryWriter();

    // destructor
    ~TelemetryWriter();

    // initialize the telemetry writer for vehicle specified by VCM
    // if 'shm' is not specified, creates a new TelemetryShm object
    // NOTE: if shm is not NULL, it should already be opened
    RetType init(VCM* vcm, TelemetryShm* shm = NULL);

    // init using use the default VCM
    // if 'shm' is not specified, creates a new TelemetryShm object
    // NOTE: if shm is not NULL, it should already be opened
    RetType init(TelemetryShm* shm = NULL);

    // write a measurement
    // NOTE: not committed to shared memory until 'flush' is called
    // NOTE: only allowed to write to virtual measurements, returns FAILURE otherwise
    RetType write(measurement_info_t* meas, uint8_t* data, size_t len);

    // write a measurement
    // NOTE: not committed to shared memory until 'flush' is called
    // NOTE: only allowed to write to virtual measurements, returns FAILURE otherwise
    RetType write(std::string& meas, uint8_t* data, size_t len);

    // flush all written measurement to shared memory
    // NOTE: writes are also logged
    RetType flush();

private:
    VCM* vcm;
    bool rm_vcm = false;

    size_t num_packets;
    uint8_t** packet_buffers;
    size_t* packet_sizes;

    bool* updated;

    TelemetryShm* shm;
    bool rm_shm = false;

    PacketLogger** loggers;
};

#endif
