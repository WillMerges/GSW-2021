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
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdarg.h>

using namespace calc;
using namespace dls;
using namespace vcm;

// Conversion functions //

// directly copy a measurement
// 'out' must be the same size as 'in'
RetType COPY(measurement_info_t* out, uint8_t* dst, ...) {
    MsgLogger logger("CALC", "COPY");

    va_list args;
    va_start(args, dst);

    measurement_info_t* in = va_arg(args, measurement_info_t*);
    uint8_t* src = va_arg(args, uint8_t*);

    va_end(args);

    if(out->size != in->size) {
        logger.log_message("Size mismatch");
        return FAILURE;
    }

    memcpy((void*)dst, (void*)src, out->size);

    return SUCCESS;
}


// Parse default virtual telemetry file
RetType parse_vfile(VCM* veh, std::vector<vcalc_t>* entries) {
    MsgLogger logger("CALC", "parse_vfile(2)");

    char* env = getenv("GSW_HOME");
    if(env == NULL) {
        logger.log_message("GSW_HOME environment variable not set!");
        return FAILURE;
    }

    std::string file = env;
    file += "data/default/virtual";

    return parse_vfile(file.c_str(), veh, entries);
}

// Parse virtual telemetry file
RetType parse_vfile(const char* filename, VCM* veh, std::vector<vcalc_t>* entries) {
    MsgLogger logger("CALC", "parse_vfile(3)");

    // open the file
    std::ifstream* f = new std::ifstream(filename);
    if(!f) {
        logger.log_message("Failed to open file");
        return FAILURE;
    }

    entries = new std::vector<vcalc_t>;
    vcalc_t entry;

    // start parsing
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

        entry.out = veh->get_info(fst);
        if(entry.out == NULL) {
            logger.log_message("No such measurement: " + fst);
            return FAILURE;
        }

        // Find our conversion function
        if(snd == "COPY") {
            entry.convert_function = &COPY;
        } else {
            logger.log_message("No such conversion function: " + snd);
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
        }

        // add the current line's entry and reset
        entries->push_back(entry);
        entry.args.clear();
    }

    f->close();

    return SUCCESS;
}
