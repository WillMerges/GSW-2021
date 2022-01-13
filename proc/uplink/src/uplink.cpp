#include "lib/dls/dls.h"
#include "lib/vcm/vcm.h"
#include "lib/nm/nm.h"
#include <signal.h>

/*
*   This executable runs the "uplink" process.
*   It uses the NetworkManager class to wait for other processes to queue
*   and then sends them over the network. The destination port is found in the
*   VCM file and the address is found in shared memory, placed there by another
*   NetworkManager instance. The 'decom' process uses several NetworkManagers
*   that all call 'Receive', which will place the address of the receiver in
*   shared memory. The NetworkInterface class is used by other processs to queue
*   messages that will be sent by the NetworkManager in the process. The name
*   of the NetworkInterface should be the device name from the VCM file.
*
*   Run as: ./uplink [config file path]
*   If no VCM config file path is specified, the default location is used
*/

using namespace dls;
using namespace vcm;
using namespace nm;


NetworkManager* net;

void sighandler(int signum) {
    MsgLogger logger("UPLINK", "sighandler");
    logger.log_message("uplink killed, shutting down");

    if(net) {
        net->Close();
    }

    exit(signum);
}


int main(int argc, char** argv) {
    MsgLogger logger("UPLINK", "main");
    logger.log_message("starting uplink process");

    // interpret the 1st argument as a config_file location if available
    std::string config_file = "";
    if(argc > 1) {
        config_file = argv[1];
    }

    VCM* veh;
    if(config_file == "") {
        veh = new VCM(); // use default config file
    } else {
        veh = new VCM(config_file); // use specified config file
    }

    // init VCM
    if(veh->init() == FAILURE) {
        logger.log_message("failed to initialize VCM");
        return -1;
    }

    // add signals
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGFPE, sighandler);
    signal(SIGABRT, sighandler);

    // send and receive from same port (but never receive as of right now)
    // ^^^ actually let the OS decide which port to bind to by calling 'bind' with a port of 0
    // net = new NetworkManager(veh->port, veh->port, (veh->device).c_str(), NULL, 0);
    net = new NetworkManager(0, veh->port, (veh->device).c_str(), NULL, 0);

    if(net->Open() == FAILURE) {
        logger.log_message("failed to open Network Manager");
        return -1;
    }

    while(1) {
        net->Send();
    }
}
