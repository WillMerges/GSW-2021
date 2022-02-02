/******************************************************************************
* Name: bindings.cpp
*
* Purpose: C bindings for CPP functions to be used for Python Ctypes
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
#include <string.h>
#include "lib/vcm/vcm.h"
#include "lib/telemetry/TelemetryViewer.h"

using namespace vcm;

extern "C" {
    TelemetryViewer* TelemetryViewer_new() {
        return new TelemetryViewer();
    }

    RetType TelemetryViewer_init0(TelemetryViewer* tv) {
        return tv->init();
    }

    RetType TelemetryViewer_init1(TelemetryViewer* tv, char* config_file) {
        std::string config_file_str = config_file;

        // leave this to be cleaned up on the stack
        // TODO this doesn't work as of now, TelemetryViewer saves the pointer not a copy
        VCM* vcm = new VCM(config_file_str);
        if(FAILURE == vcm->init()) {
            return FAILURE;
        }

        return tv->init(vcm);
    }

    void TelemetryViewer_remove_all(TelemetryViewer* tv) {
        tv->remove_all();
    }

    RetType TelemetryViewer_add_all(TelemetryViewer* tv) {
        return tv->add_all();
    }

    RetType TelemetryViewer_add0(TelemetryViewer* tv, char* measurement) {
        std::string meas_str = measurement;

        return tv->add(meas_str);
    }

    RetType TelemetryViewer_add1(TelemetryViewer* tv, int packet_id) {
        return tv->add((unsigned int)packet_id);
    }

    void TelemetryViewer_set_update_standard(TelemetryViewer* tv) {
        tv->set_update_mode(TelemetryViewer::STANDARD_UPDATE);
    }

    void TelemetryViewer_set_update_blocking(TelemetryViewer* tv) {
        tv->set_update_mode(TelemetryViewer::BLOCKING_UPDATE);
    }

    void TelemetryViewer_set_update_nonblocking(TelemetryViewer* tv) {
        tv->set_update_mode(TelemetryViewer::NONBLOCKING_UPDATE);
    }

    RetType TelemetryViewer_update(TelemetryViewer* tv) {
        return tv->update();
    }

    // TODO figure out a way of error checking the 'get' functions

    const char* TelemetryViewer_get_str(TelemetryViewer* tv, uint32_t* measurement) {
        // python is stupid and cant figure out string encoding, so we get each character as a 32 bit value
        std::string meas_str = "";
        while(*measurement) {
            meas_str += *((char*)measurement);
            measurement++;
        }

        std::string* ret = new std::string;
        if(FAILURE == tv->get_str(meas_str, ret)) {
            return "";
        }

        return ret->c_str();
    }

    float TelemetryViewer_get_float(TelemetryViewer* tv, char* measurement) {
        std::string meas_str = measurement;

        float ret;
        if(FAILURE == tv->get_float(meas_str, &ret)) {
            return 0.0;
        }

        return ret;
    }

    double TelemetryViewer_get_double(TelemetryViewer* tv, char* measurement) {
        std::string meas_str = measurement;

        double ret;
        if(FAILURE == tv->get_double(meas_str, &ret)) {
            return 0.0;
        }

        return ret;
    }

    int TelemetryViewer_get_int(TelemetryViewer* tv, char* measurement) {
        std::string meas_str = measurement;

        int ret;
        if(FAILURE == tv->get_int(meas_str, &ret)) {
            return 0;
        }

        return ret;
    }

    unsigned int TelemetryViewer_get_uint(TelemetryViewer* tv, char* measurement) {
        std::string meas_str = measurement;

        unsigned int ret;
        if(FAILURE == tv->get_uint(meas_str, &ret)) {
            return 0;
        }

        return ret;
    }
}
