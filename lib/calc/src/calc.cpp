/********************************************************************
*  Name: calc.h
*
*  Purpose: Functions to handle calculating virtual telemetry values.
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include "lib/calc/calc.h"
#include "lib/dls/dls.h"
#include "lib/convert/convert.h"
#include <fstream>
#include <sstream>
#include <string.h>

using namespace calc;
using namespace dls;
using namespace vcm;
using namespace convert;

// static data //
VCM* veh = NULL;

// Conversion functions //

// directly copy a measurement
// 'out' must be the same size as 'in'
RetType COPY(measurement_info_t* meas, uint8_t* dst, std::vector<arg_t>& args) {
    MsgLogger logger("CALC", "COPY");

    if(args.size() != 1) {
        logger.log_message("Incorrect number of args");
        return FAILURE;
    }

    measurement_info_t* in = args[0].meas;
    const uint8_t* src = args[0].addr;

    if(meas->size != in->size) {
        logger.log_message("Size mismatch");
        return FAILURE;
    }

    // need a special function for writing single measurement to shared memory!
    memcpy((void*)(dst), (void*)src, meas->size);

    return SUCCESS;
}

RetType SUM_UINT(measurement_info_t* meas, uint8_t* dst, std::vector<arg_t>& args) {
    MsgLogger logger("CALC", "INTSUM");

    uint32_t sum = 0;
    uint32_t temp;

    // sum all arguments
    for(arg_t arg : args) {
        if(convert_uint(veh, arg.meas, arg.addr, &temp) != SUCCESS) {
            logger.log_message("conversion failed");
            return FAILURE;
        }

        sum += temp;
    }

    uint8_t* sumb = (uint8_t*)&sum;

    // write out sum to output measurement

    // TODO find a way around endianness reversing?
    // e.g. for all virtual measurements, use system endianness (would mean VCM would have to check a virtual measurement is ONLY in virtual packets)
    if(veh->sys_endianness != veh->recv_endianness) {
        for(size_t i = 0; i < sizeof(uint32_t); i++) {
            dst[meas->size - i] = sumb[i];
        }

        // zero the rest
        memset(dst, 0, meas->size - sizeof(uint32_t));
    } else {
        for(size_t i = 0; i < sizeof(uint32_t); i++) {
            dst[i] = sumb[i];
        }
    }

    return SUCCESS;
}

// define hash function for vcalc_t
namespace std {
    template<>
    struct hash<vcalc_t> {
        inline size_t operator()(const vcalc_t& v) const {
            return v.unique_id;
        }
    };
}

// Parse virtual telemetry file
RetType calc::parse_vfile(VCM* veh, std::vector<vcalc_t>* entries) {
    MsgLogger logger("CALC", "parse_vfile");

    // open the file
    std::ifstream* f = new std::ifstream(veh->vcalc_file.c_str());
    if(!f) {
        logger.log_message("Failed to open file");
        return FAILURE;
    }

    vcalc_t entry;

    // start parsing
    size_t unique_id = 0;
    for(std::string line; std::getline(*f,line); ) {
        if(line == "" || !line.rfind("#",0)) { // blank or comment '#'
            continue;
        }

        // The first two strings are non-optional
        std::istringstream ss(line);
        std::string fst;
        ss >> fst;
        std::string snd;
        ss >> snd;

        if(fst == "" || snd == "") {
            logger.log_message("Missing information on line: " + line);
            return FAILURE;
        }

        // first string is output measurement
        entry.out = veh->get_info(fst);
        if(entry.out == NULL) {
            logger.log_message("No such measurement: " + fst);
            return FAILURE;
        }

        // Find our conversion function
        if(snd == "COPY") {
            entry.convert_function = &COPY;
        } else if(snd == "SUM_UINT") {
            entry.convert_function = &SUM_UINT;
        } else {
            logger.log_message("No such conversion function: " + snd);
            return FAILURE;
        }

        // The rest of the tokens are arguments
        std::string next;
        ss >> next;
        measurement_info_t* info = NULL;
        while(next != "" && next[0] != '#') {
            info = veh->get_info(next);

            if(!info) {
                logger.log_message("No such measurement: " + next);
                return FAILURE;
            }

            entry.args.push_back(info);

            next = "";
            ss >> next;
        }

        // add a unique id to this entry
        entry.unique_id = unique_id;
        unique_id++;

        // add the current line's entry and reset
        entries->push_back(entry);
        entry.args.clear();
    }

    f->close();

    return SUCCESS;
}
