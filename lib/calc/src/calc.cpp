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
#include "lib/calc/vfunc.h"
#include <fstream>
#include <sstream>

using namespace calc;
using namespace dls;
using namespace vcm;
using namespace convert;

// globally defined
VCM* veh = NULL;

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

    // set the global VCM to the passed in one
    ::veh = veh;

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
        entry.convert_function = NULL;
        vfunc_t vfunc;
        size_t i = 0;
        while(1) {
            vfunc = vfunc_list[i];

            if(vfunc.name == NULL && vfunc.func == NULL) {
                // end of list
                break;
            }

            if(snd == vfunc.name) {
                entry.convert_function = vfunc.func;
                break;
            }

            i++;
        }

        if(!entry.convert_function) {
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
