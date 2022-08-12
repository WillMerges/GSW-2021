/*******************************************************************************
* Name: TelemetryViewer.cpp
*
* Purpose: Top level read access to telemetry measurements
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
#include <string.h>
#include <limits.h>

#include "lib/telemetry/TelemetryViewer.h"
#include "lib/dls/dls.h"
#include "lib/convert/convert.h"

using namespace dls;
using namespace convert;


TelemetryViewer::TelemetryViewer() {
    update_mode = STANDARD_UPDATE;
    packet_ids = NULL;
    num_packets = 0;
    vcm = NULL;
    packet_sizes = NULL;
    packet_buffers = NULL;
}

TelemetryViewer::~TelemetryViewer() {
    if(shm != NULL) {
        if(shm->read_locked) {
            // if we currently hold a lock we need to release it before exiting so we don't block everyone
            // most processes will catch signals so this should not happen
            // most processes ignore immediate kills and unlock before exiting
            // if they are stuck blocking in read_lock they call 'sighandler' to release them and then they exit, so the lock is not held
            shm->read_unlock();
        }

        if(rm_shm) {
            delete shm;
        }
    }

    if(packet_ids != NULL) {
        delete packet_ids;
    }

    if(packet_buffers != NULL) {
        for(size_t i = 0; i < num_packets; i++) {
            if(packet_buffers[i] != NULL) {
                delete packet_buffers[i];
            }
        }

        delete packet_buffers;
    }

    if(packet_sizes != NULL) {
        delete packet_sizes;
    }

    if(vcm && rm_vcm) {
        delete vcm;
    }
}

RetType TelemetryViewer::init(TelemetryShm* shm) {
    MsgLogger logger("TelemetryViewer", "init");

    VCM* vcm = new VCM();
    rm_vcm = true;

    if(FAILURE == vcm->init()) {
        logger.log_message("Failed to init default VCM");
        return FAILURE;
    }

    return init(vcm, shm);
}

RetType TelemetryViewer::init(VCM* vcm, TelemetryShm* shm) {
    MsgLogger logger("TelemetryViewer", "init");

    if(shm == NULL) {
        this->shm = new TelemetryShm();
        rm_shm = true;

        if(this->shm->init(vcm) == FAILURE) {
            logger.log_message("failed to initialize telemetry shared memory");
            return FAILURE;
        }

        if(this->shm->attach() == FAILURE) {
            logger.log_message("failed to attach to telemetry shared memory");
            return FAILURE;
        }
    } else {
        this->shm = shm;
    }

    this->vcm = vcm;

    packet_ids = new unsigned int[vcm->num_packets];
    packet_sizes = new size_t[vcm->num_packets];
    packet_buffers = new uint8_t*[vcm->num_packets];
    memset(packet_buffers, 0, sizeof(uint8_t*) * vcm->num_packets);

    return SUCCESS;
}

void TelemetryViewer::remove_all() {
    check_all = false;
    num_packets = 0;

    // TODO free packet_buffers? currently leaves allocated until destruction
}

RetType TelemetryViewer::add_all() {
    check_all = true;

    // NOTE: this relies on the fact packet ids are sequential indices
    // we still want to add these so that we create the necessary buffers etc.
    for(unsigned int id = 0; id < vcm->num_packets; id++) {
        if(FAILURE == add(id)) {
            return FAILURE;
        }
    }

    return SUCCESS;
}

RetType TelemetryViewer::add(measurement_info_t* info) {
    MsgLogger logger("TelemetryViewer", "add");

    if(info == NULL) {
        logger.log_message("Invalid measurement, cannot add");
        return FAILURE;
    }

    for(location_info_t loc : info->locations) {
        if(FAILURE == add(loc.packet_index)) {
            logger.log_message("failed to add measurement");
            return FAILURE;
        }
    }

    return SUCCESS;
}

RetType TelemetryViewer::add(std::string& measurement) {
    measurement_info_t* info = vcm->get_info(measurement);
    return add(info);
}

RetType TelemetryViewer::add(uint32_t packet_id) {
    if(packet_id < vcm->num_packets) {
        for(size_t i = 0; i < num_packets; i++) {
            if(packet_ids[i] == packet_id) {
                // packet is already being tracked
                return SUCCESS;
            }
        }

        packet_ids[num_packets] = packet_id;
        packet_sizes[packet_id] = vcm->packets[packet_id]->size;
        packet_buffers[packet_id] = new uint8_t[packet_sizes[packet_id]];
        memset(packet_buffers[packet_id], 0, packet_sizes[packet_id]); // zero buffer
        num_packets++;

        return SUCCESS;
    }

    MsgLogger logger("TelemetryViewer", "add");
    logger.log_message("invalid packet id");
    return FAILURE;
}

void TelemetryViewer::set_update_mode(update_mode_t mode) {
    update_mode = mode;

    TelemetryShm::read_mode_t shm_mode;
    switch(update_mode) {
        case STANDARD_UPDATE:
            shm_mode = TelemetryShm::STANDARD_READ;
            break;
        case BLOCKING_UPDATE:
            shm_mode = TelemetryShm::BLOCKING_READ;
            break;
        case NONBLOCKING_UPDATE:
            shm_mode = TelemetryShm::NONBLOCKING_READ;
            break;
    }

    shm->set_read_mode(shm_mode);
}

RetType TelemetryViewer::update(uint32_t timeout) {
    MsgLogger logger("TelemetryViewer", "update");

    RetType status;
    if(check_all) {
        status = shm->read_lock(timeout);
        if(status != SUCCESS) {
            return status; // could be BLOCKED or FAILURE or TIMEOUT
        }
    } else {
        status = shm->read_lock(packet_ids, num_packets, timeout);
        if(status != SUCCESS) {
            return status; // could be BLOCKED or FAILURE or TIMEOUT
        }
    }

    // copy in packets we're tracking from shared memory
    unsigned int id;
    for(size_t i = 0; i < num_packets; i++) {
        id = packet_ids[i];
        if(shm->updated[id]) {
            memcpy(packet_buffers[id], shm->get_buffer(id), packet_sizes[id]);
        } // if the packet didn't update, save ourself the copy
    }

    // unlock shared memory
    if(FAILURE == shm->read_unlock()) {
        logger.log_message("failed to unlock shared memory");
        return FAILURE;
    }

    return SUCCESS;
}

void TelemetryViewer::sighandler() {
    shm->sighandler();
}

RetType TelemetryViewer::latest_data(measurement_info_t* meas, uint8_t** data) {
    MsgLogger logger("TelemetryViewer", "latest_data");

    std::vector<location_info_t> locs = meas->locations;

    if(locs.size() <= 0) {
        logger.log_message("measurement does not exist anywhere");
        return FAILURE;
    }

    location_info_t* best_loc;
    long int best = INT_MAX;
    uint32_t curr;
    for(size_t i = 0; i < locs.size(); i++) {
        if(shm->update_value(locs[i].packet_index, &curr) == FAILURE) {
            logger.log_message("unable to retrieve update value for packet");
            return FAILURE;
        }

        if(curr < best) {
            best = curr;
            best_loc = &(locs[i]);
        }
    }

    // guaranteed loc is not NULL since we found some location
    *data = packet_buffers[best_loc->packet_index] + best_loc->offset;

    return SUCCESS;
}

bool TelemetryViewer::updated(measurement_info_t* meas) {
    // MsgLogger logger("TelemetryViewer", "updated");

    // loop through every packet this measurement is in
    // TODO this could be slow for a lot of lookups, better solution?
    for(location_info_t loc : meas->locations) {
        // no need to check for bounds
        // all indices in VCM are guaranteed to be in bounds
        if(shm->updated[loc.packet_index]) {
            // we found an updated packet
            return true;
        }
    }

    // otherwise no packets this measurement is in was updated
    return false;
}

RetType TelemetryViewer::get_str(measurement_info_t* meas, std::string* val) {
    MsgLogger logger("TelemetryViewer", "get");

    uint8_t* data;
    if(latest_data(meas, &data) == FAILURE) {
        logger.log_message("failed to locate latest data for measurement");
        return FAILURE;
    }

    if(convert_to(vcm, meas, data, val) == FAILURE) {
        logger.log_message("failed to convert measurement to string");
        return FAILURE;
    }

    return SUCCESS;
}

RetType TelemetryViewer::get_float(measurement_info_t* meas, float* val) {
    MsgLogger logger("TelemetryViewer", "get");

    uint8_t* data;
    if(latest_data(meas, &data) == FAILURE) {
        logger.log_message("failed to locate latest data for measurement");
        return FAILURE;
    }

    if(convert_to(vcm, meas, data, val) == FAILURE) {
        logger.log_message("failed to convert measurement to float");
        return FAILURE;
    }

    return SUCCESS;
}

RetType TelemetryViewer::get_double(measurement_info_t* meas, double* val) {
    MsgLogger logger("TelemetryViewer", "get");

    uint8_t* data;
    if(latest_data(meas, &data) == FAILURE) {
        logger.log_message("failed to locate latest data for measurement");
        return FAILURE;
    }

    if(convert_to(vcm, meas, data, val) == FAILURE) {
        logger.log_message("failed to convert measurement to double");
        return FAILURE;
    }

    return SUCCESS;
}

RetType TelemetryViewer::get_int(measurement_info_t* meas, int* val) {
    MsgLogger logger("TelemetryViewer", "get");

    uint8_t* data;
    if(latest_data(meas, &data) == FAILURE) {
        logger.log_message("failed to locate latest data for measurement");
        return FAILURE;
    }

    if(convert_to(vcm, meas, data, val) == FAILURE) {
        logger.log_message("failed to convert measurement to int");
        return FAILURE;
    }

    return SUCCESS;
}

RetType TelemetryViewer::get_uint(measurement_info_t* meas, unsigned int* val) {
    MsgLogger logger("TelemetryViewer", "get");

    uint8_t* data;
    if(latest_data(meas, &data) == FAILURE) {
        logger.log_message("failed to locate latest data for measurement");
        return FAILURE;
    }

    if(convert_to(vcm, meas, data, val) == FAILURE) {
        logger.log_message("failed to convert measurement to uint");
        return FAILURE;
    }

    return SUCCESS;
}

RetType TelemetryViewer::get_raw(measurement_info_t* meas, uint8_t* buffer) {
    uint8_t* data;
    if(latest_data(meas, &data) == FAILURE) {
        MsgLogger logger("TelemetryViewer", "get_raw");
        logger.log_message("failed to locate latest data for measurement");
        return FAILURE;
    }

    // copy the data out
    memcpy(buffer, data, meas->size);

    // return meas->size;
    return SUCCESS;
}

RetType TelemetryViewer::get_raw(measurement_info_t* meas, uint8_t** buffer) {
    if(latest_data(meas, buffer) == FAILURE) {
        MsgLogger logger("TelemetryViewer", "get_raw");
        logger.log_message("failed to locate latest data for measurement");

        return FAILURE;
    }

    return SUCCESS;
}

RetType TelemetryViewer::get_str(std::string& meas, std::string* val) {
    MsgLogger logger("TelemetryViewer", "get");

    measurement_info_t* m_info = vcm->get_info(meas);
    if(m_info == NULL) {
        logger.log_message("Measurement not found: " + meas);
        return FAILURE;
    }

    return get_str(m_info, val);
}

RetType TelemetryViewer::get_float(std::string& meas, float* val) {
    MsgLogger logger("TelemetryViewer", "get");

    measurement_info_t* m_info = vcm->get_info(meas);
    if(m_info == NULL) {
        logger.log_message("Measurement not found: " + meas);
        return FAILURE;
    }

    return get_float(m_info, val);
}

RetType TelemetryViewer::get_double(std::string& meas, double* val) {
    MsgLogger logger("TelemetryViewer", "get");

    measurement_info_t* m_info = vcm->get_info(meas);
    if(m_info == NULL) {
        logger.log_message("Measurement not found: " + meas);
        return FAILURE;
    }

    return get_double(m_info, val);
}

RetType TelemetryViewer::get_int(std::string& meas, int* val) {
    MsgLogger logger("TelemetryViewer", "get");

    measurement_info_t* m_info = vcm->get_info(meas);
    if(m_info == NULL) {
        logger.log_message("Measurement not found: " + meas);
        return FAILURE;
    }

    return get_int(m_info, val);
}

RetType TelemetryViewer::get_uint(std::string& meas, unsigned int* val) {
    MsgLogger logger("TelemetryViewer", "get");

    measurement_info_t* m_info = vcm->get_info(meas);
    if(m_info == NULL) {
        logger.log_message("Measurement not found: " + meas);
        return FAILURE;
    }

    return get_uint(m_info, val);
}

RetType TelemetryViewer::get_raw(std::string& meas, uint8_t* buffer) {
    MsgLogger logger("TelemetryViewer", "get_raw");

    measurement_info_t* m_info = vcm->get_info(meas);
    if(m_info == NULL) {
        logger.log_message("Measurement not found: " + meas);
        return FAILURE;
    }

    return get_raw(m_info, buffer);
}
