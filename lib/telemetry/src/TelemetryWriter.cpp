/*******************************************************************************
* Name: TelemetryWriter.cpp
*
* Purpose: Top level write access to telemetry measurements
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
#include <string.h>

#include "lib/telemetry/TelemetryWriter.h"
#include "lib/telemetry/TelemetryShm.h"
#include "lib/dls/dls.h"

using namespace vcm;
using namespace dls;

// TODO add writing to individual packets
//      currently all virtual packets are added when 'init' is called
//      so anytime 'lock' is called, all packets are locked, even though the user
//      may not write to all of them

// constructor
TelemetryWriter::TelemetryWriter() {
    vcm = NULL;
    packet_buffers = NULL;
    num_packets = 0;
    shm = NULL;
}

// destructor
TelemetryWriter::~TelemetryWriter() {
}

RetType TelemetryWriter::init(std::shared_ptr<TelemetryShm> shm) {
    MsgLogger logger("TelemetryWriter", "init (1 arg)");

    auto vcm = std::make_shared<VCM>();
    rm_vcm = true;

    if(SUCCESS != vcm->init()) {
        logger.log_message("failed to init default VCM");
        return FAILURE;
    }

    return init(std::move(vcm), std::move(shm));
}

RetType TelemetryWriter::init(std::shared_ptr<VCM> vcm, std::shared_ptr<TelemetryShm> shm) {
    MsgLogger logger("TelemetryWriter", "init (2 args)");

    if(shm == nullptr) {
        this->shm = std::make_unique<TelemetryShm>();
        rm_shm = true;

        if(this->shm->init(vcm.get()) == FAILURE) {
            logger.log_message("failed to initialize telemetry shared memory");
            return FAILURE;
        }

        if(this->shm->open() == FAILURE) {
            logger.log_message("failed to attach to telemetry shared memory");
            return FAILURE;
        }
    } else {
        this->shm = shm;
    }

    this->vcm = vcm;

    // create buffers for each virtual packet
    num_packets = vcm->num_packets;
    packet_sizes = std::make_unique<size_t>(num_packets);
    packet_buffers = std::make_unique<uint8_t*[]>(num_packets);
    updated = std::make_unique<bool[]>(num_packets);
    loggers = std::make_unique<PacketLogger*[]>(num_packets);

    for(size_t i = 0; i < num_packets; i++) {
        if(vcm->packets[i]->is_virtual) {
            packet_buffers[i] = new uint8_t[vcm->packets[i]->size];
            // zero out the buffer
            memset(packet_buffers[i], 0x0, vcm->packets[i]->size);
            packet_sizes.get()[i] = vcm->packets[i]->size;
            loggers[i] = new PacketLogger(vcm->device + "(" + std::to_string(i) + ")");
        } else {
            packet_buffers[i] = NULL;
            packet_sizes.get()[i] = 0;
            loggers[i] = nullptr;
        }

        updated[i] = false;
    }

    return SUCCESS;
}

// helper function to copy data into telemetry
// basically memcpy with endianness checking
void TelemetryWriter::telemetry_copy(measurement_info_t* meas, uint8_t* dst, const uint8_t* src, size_t len) {
    if(vcm->sys_endianness != meas->endianness) {
        // copy backwards
        for(size_t i = 0; i < len; i++) {
            dst[len - i - 1] = src[i];
        }
    } else {
        for(size_t i = 0; i < len; i++) {
            dst[i] = src[i];
        }
    }
}

RetType TelemetryWriter::write(measurement_info_t* meas, uint8_t* data, size_t len) {
    if(len != meas->size) {
        MsgLogger logger("TelemetryWriter", "write");
        logger.log_message("must write size of measurement");

        return FAILURE;
    }

    // TODO is there a more efficient way than checking each packet every write?
    // could store which measurements go where at init
    // this isn't so bad for now though
    // tradeoff between using something like a hashmap and this
    // if we assume a virtual measurement is only in virtual packets, this is faster since we avoid a lookup
    for(location_info_t loc : meas->locations) {
        if(vcm->packets[loc.packet_index]->is_virtual) {
            telemetry_copy(meas, packet_buffers[loc.packet_index] + loc.offset, data, len);
            updated[loc.packet_index] = true;
        }
    }

    return SUCCESS;
}

RetType TelemetryWriter::write(std::string& meas, uint8_t* data, size_t len) {
    measurement_info_t* m = vcm->get_info(meas);

    if(m == NULL) {
        MsgLogger logger("TelemetryWriter", "write (string)");
        logger.log_message("no such measurement");

        return FAILURE;
    }

    return write(m, data, len);
}

RetType TelemetryWriter::write_raw(measurement_info_t* meas, uint8_t* data, size_t len) {
    if(len > meas->size) {
        MsgLogger logger("TelemetryWriter", "write_raw");
        logger.log_message("must write less than size of measurement");

        return FAILURE;
    }

    for(location_info_t loc : meas->locations) {
        if(vcm->packets[loc.packet_index]->is_virtual) {
            memcpy(packet_buffers[loc.packet_index] + loc.offset, data, len);
            updated[loc.packet_index] = true;
        }
    }

    return SUCCESS;
}

RetType TelemetryWriter::flush() {
    int ret = SUCCESS;

    // TODO is there a better way to keep track of which packets updated?
    // so we don't have to loop through all of them
    for(size_t i = 0; i < num_packets; i++) {
        if(updated[i]) {
            // write to logger
            ret |= loggers[i]->log_packet(packet_buffers[i], packet_sizes.get()[i]);

            // write to shared mem
            // TODO it would be faster if each call to 'write' updated the underlying shm buffer (since it's locked, we can do that)
            ret |= shm->write(i, packet_buffers[i]);

            updated[i] = false;
        }
    }

    return (RetType)ret;
}

RetType TelemetryWriter::lock(bool check_for_updates) {
    // TODO it may be better to have a list of all the virtual packets
    // see same complaint in 'flush'
    for(size_t i = 0; i < num_packets; i++) {
        if(packet_sizes.get()[i]) { // if we have a packet size, it's a virtual packet
            if(SUCCESS != shm->write_lock(i)) {
                return FAILURE;
            }

            // copy the current contents of the packet to our cache
            // we want to not overwrite someone else's write with what's in our cache
            // NOTE: shm->updated could be true because the same tshm is being used by a TelemetryViewer that called 'update'
            // TODO efficiency issue: us flushing our buffers causes update to be true, but we already have that update cached
            //      ^^^ hard problem, can't just not set updated in the telemetryshm, any viewer won't see our updates
            //      need to know if we changed it, or someone else did
            //      ^^^ keep track of the nonce from our last flush and see if it matches the current nonce
            if(check_for_updates || shm->updated[i]) {
                memcpy(packet_buffers[i], (void*)(shm->get_buffer(i)), packet_sizes.get()[i]);
            }
        }
    }

    return SUCCESS;
}

RetType TelemetryWriter::unlock() {
    // TODO it may be better to have a list of all the virtual packets
    // see same complaint in 'flush' and 'lock_all'
    for(size_t i = 0; i < num_packets; i++) {
        if(packet_sizes.get()[i]) { // if we have a packet size, it's a virtual packet
            if(SUCCESS != shm->write_unlock(i)) {
                return FAILURE;
            }
        }
    }

    return SUCCESS;
}
