/********************************************************************
*  Name: trigger.cpp
*
*  Purpose: Functions that handle measurement triggers.
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include "lib/trigger/trigger.h"
#include "lib/trigger/map.h"
#include "lib/dls/dls.h"
#include "common/types.h"
#include <fstream>
#include <sstream>
#include <string>

using namespace dls;

namespace std {
    template<>
    struct hash<trigger_t> {
        inline size_t operator()(const trigger_t& t) const {
            return t.unique_id;
        }
    };
}

RetType trigger::parse_trigger_file(VCM* veh, std::vector<trigger_t>* triggers) {
    MsgLogger logger("TRIGGER", "parse_trigger_file");

    // open the file
    std::ifstream* f = new std::ifstream(veh->trigger_file.c_str());
    if(!f) {
        logger.log_message("Failed to open file");
        return FILENOTFOUND;
    }

    trigger_t trigger;

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

        // find the trigger measurement
        trigger.meas = veh->get_info(fst);
        if(trigger.meas == NULL) {
            logger.log_message("No such measurement: " + fst);
            return FAILURE;
        }

        // find the trigger function
        trigger.func = NULL;
        trigger_map_t t;
        size_t i = 0;
        while(1) {
            t = trigger_func_list[i];

            if(t.func == NULL && t.name == NULL) {
                // end of list
                break;
            }

            if(t.name == snd) {
                // this is our function
                trigger.func = t.func;
                break;
            }

            i++;
        }

        if(trigger.func == NULL) {
            logger.log_message("No such trigger function: " + snd);
            return FAILURE;
        }

        // build argument list
        arg_t args;
        args.args = NULL;
        args.num_args = 0;
        std::string tok;


        while(!ss.eof()) {
            ss >> tok;

            measurement_info_t* m = veh->get_info(tok);

            if(m == NULL) {
                logger.log_message("No such measurement: " + tok);
                return FAILURE;
            }

            args.num_args++;
            args.args = (measurement_info_t**)realloc((void*)args.args, sizeof(measurement_info_t*) * args.num_args);
            args.args[args.num_args - 1] = m;
        }

        if(args.num_args < t.min_args) {
            logger.log_message("too few arguments for trigger: " + std::string(t.name));
            return FAILURE;
        }

        if(args.num_args > t.min_args) {
            logger.log_message("too many arguments for trigger: " + std::string(t.name));
            return FAILURE;
        }

        trigger.args = args;

        // add unique id
        trigger.unique_id = unique_id;
        unique_id++;

        // save this trigger
        triggers->push_back(trigger);
    }

    f->close();

    return SUCCESS;
}
