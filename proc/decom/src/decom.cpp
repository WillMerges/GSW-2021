#include <stdio.h>
#include <csignal>
#include "lib/nm/nm.h"
#include "lib/shm/shm.h"
// #include "lib/ldms/ldms.h"
#include "lib/dls/dls.h"
#include "lib/vcm/vcm.h"
#include "common/types.h"
#include <csignal>
#include <string>

// using namespace ldms;
using namespace dls;
using namespace vcm;
using namespace nm;
using namespace shm;

// TelemetryParser* tp = NULL;
NetworkManager* net = NULL;

void sighandler(int signum) {
    MsgLogger logger("DECOM");
    logger.log_message("decom killed, cleaning up resources");

    // if(tp) {
    //     delete tp;
    // }
    if(net) {
        delete net; // this also calls close
    }

    exit(signum);
}

int main(int argc, char** argv) {
    // interpret the 1st argument as a config_file location if available
    std::string config_file = "";
    if(argc > 1) {
        config_file = argv[1];
    }

    MsgLogger logger("DECOM");
    logger.log_message("starting decom");

    // can't catch sigkill or sigstop though
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGFPE, sighandler);
    signal(SIGABRT, sighandler);

    // if(config_file == "") {
    //     tp = ldms::create_default_parser();
    // } else {
    //     tp = ldms::create_parser(config_file);
    // }
    // if(!tp) {
    //     printf("Unable to create parser\n");
    //     return -1;
    // }
    // tp->Parse();

    VCM* vcm = NULL;
    if(config_file == "") {
        vcm = new VCM(); // use default config file
    } else {
        vcm = new VCM(config_file); // use specified config file
    }

    net = new NetworkManager(vcm);
    if(FAILURE == net->Open()) {
        logger.log_message("failed to open network manager");
        return -1;
    }

    // attach to shared memory
    if(FAILURE == attach_to_shm(vcm)) {
        logger.log_message("unable to attach to shared memory");
        return FAILURE;
    }

    // clear shared memory
    if(FAILURE == clear_shm()) {
        logger.log_message("unable to clear shared memory");
        return FAILURE;
    }


    PacketLogger plogger(vcm->device);
    while(1) {
        //if(SUCCESS != net->Iterate()) {
            // do something maybe
        //}

        // send any outgoing messages
        net->Send(); // don't care about the return

        // read any incoming message and write it to shared memory
        if(SUCCESS == net->Receive()) {
            if(net->in_size != vcm->packet_size) {
                logger.log_message("Packet size mismatch, " + std::to_string(vcm->packet_size) +
                                   " != " + std::to_string(net->in_size) + " (received)");
            } else { // only write the packet to shared mem if it's the correct size
                write_to_shm((void*)net->in_buffer, net->in_size);
            }
            plogger.log_packet((unsigned char*)net->in_buffer, net->in_size); // log the packet anyways
        }
    }
}
