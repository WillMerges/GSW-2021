/*******************************************************************************
* Name: TelemetryWriter.h
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
    RetType init(std::shared_ptr<VCM> vcm, boost::interprocess::offset_ptr<TelemetryShm> shm = nullptr);

    // init using use the default VCM
    // if 'shm' is not specified, creates a new TelemetryShm object
    // NOTE: if shm is not NULL, it should already be opened
    RetType init(boost::interprocess::offset_ptr<TelemetryShm> shm = nullptr);

    // write a measurement
    // NOTE: not committed to shared memory until 'flush' is called
    // NOTE: only allowed to write to virtual measurements, returns FAILURE otherwise
    RetType write(measurement_info_t* meas, uint8_t* data, size_t len);

    // write a measurement
    // NOTE: not committed to shared memory until 'flush' is called
    // NOTE: only allowed to write to virtual measurements, returns FAILURE otherwise
    RetType write(std::string& meas, uint8_t* data, size_t len);

    // write but does not switch endianness
    RetType write_raw(measurement_info_t* meas, uint8_t* data, size_t len);

    // flush all written measurement to shared memory
    // NOTE: writes are also logged
    RetType flush();

    // lock all virtual packets
    // if check_for_updates is true, all packets are copied to the local cache
    // if it's false, it uses the packets updated the last time 'read_lock' was called to update to the cache
    // NOTE: blocking
    RetType lock(bool check_for_updates=true);

    // unlock all virtual packets
    RetType unlock();

private:
    std::shared_ptr<VCM> vcm;
    bool rm_vcm = false;

    size_t num_packets;
    std::unique_ptr<uint8_t*[]> packet_buffers;
    std::unique_ptr<size_t> packet_sizes;

    std::unique_ptr<bool[]> updated;

    boost::interprocess::offset_ptr<TelemetryShm> shm;
    bool rm_shm = false;

//    PacketLogger** loggers;
    std::unique_ptr<PacketLogger*[]> loggers;

    void telemetry_copy(measurement_info_t* meas, uint8_t* dst, const uint8_t* src, size_t len);
};

#endif
