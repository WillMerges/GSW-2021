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
#include "lib/dls/dls.h"
#include "lib/vcm/vcm.h"
#include "lib/nm/nm.h"
#include "common/types.h"
#include <signal.h>
#include <vector>
#include <csignal>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace dls;
using namespace vcm;
using namespace nm;

VCM* veh = NULL;

NetworkTransmitter* net;

bool child_proc = false;
bool ignore_kill = false;
bool killed = false;

int received_sig = 0;

std::vector<pid_t> pids;

std::string uplink_id = "UPLINK[master]";


void sighandler(int signum) {
    MsgLogger logger(uplink_id.c_str(), "sighandler");

    if(ignore_kill) {
        logger.log_message("igorning signal");
        return;
    }

    if(!child_proc) { // if we're the main process, kill all the children :(
        logger.log_message("reaping children sub-processes");

        size_t i = 0;
        for(pid_t pid : pids) {
            if(-1 == kill(pid, SIGTERM)) {
                logger.log_message("failed to kill UPLINK[" + std::to_string(i) +
                                    "] child with PID: " + std::to_string(pid));
                // ignore and kill the other children
            } else {
                logger.log_message("killed UPLINK[" + std::to_string(i) +
                                    "] child with PID: " + std::to_string(pid));
            }
            i++;
        }

        logger.log_message("killed all children, exiting");
        exit(signum);
    } else { // if we're a child we need to clean up our mess
        // mark us as killed, don't want to kill immediately in case we have shared memory locked
        logger.log_message("child process received kill signal");

        killed = true;
        received_sig = signum;
    }
}

void child_cleanup() {
    MsgLogger logger(uplink_id.c_str(), "child_cleanup");

    logger.log_message("killed, cleaning up resources");
    if(net) { // it's possible we get killed before we can create our network manager
        delete net; // this also calls close
    }
}

// main logic for each subprocess
// only return if something bad happens
void execute(std::string device_name) {
    uplink_id = "UPLINK[" + device_name + "]";

    MsgLogger logger(uplink_id.c_str(), "execute");

    net = new NetworkTransmitter();

    if(SUCCESS != net->init(device_name, veh, 0)) {
        logger.log_message("failed to initialize network transmitter");
        return;
    }

    while(!killed) {
        net->tx();
    }

    child_cleanup();
    exit(received_sig);
}


int main(int argc, char** argv) {
    MsgLogger logger(uplink_id.c_str(), "main");
    logger.log_message("starting uplink master process");

    // interpret the 1st argument as a config_file location if available
    std::string config_file = "";
    if(argc > 1) {
        config_file = argv[1];
    }

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

    logger.log_message("starting uplink sub-processes");

    // we don't want to get killed while spawning children
    // we may lose track of a child if we do
    ignore_kill = true;

    pid_t pid;
    size_t i = 0;
    for(std::string device : veh->net_devices) {

        pid = fork();
        if(pid == -1) {
            logger.log_message("failed to start uplink sub-process UPLINK[" + device + "]");
        } else if(pid == 0) { // we are the child
            // if we allowed killing during this step, the child process could be killed before it's able to set the 'child_proc' boolean
            // which would make it try and clean up other children on kill, and we dont want multiple processes trying to kill each other
            child_proc = true;
            ignore_kill = false;
            execute(device);

            return -1; // if a child returns, something bad happened to it and it should exit
        }

        // otherwise we're the parent, keep going
        logger.log_message("started uplink sub-process UPLINK[" + device
                            + "] with PID: " + std::to_string(pid));
        i++;
        pids.push_back(pid);
    }

    ignore_kill = false;

    // monitor children processes in case they die
    while(1) {
        pid = wait(NULL);
        if(pid == -1) { // error
            if(errno == ECHILD) { // no more children left to wait for
                logger.log_message("all uplink sub-process children have died, exiting unexpectedly");
                return -1;
            }
        }

        logger.log_message("uplink sub-process child with PID: " + std::to_string(pid) + " died unexpectedly");
        // continue and keep monitoring other children
    }

    // should never get here
    return -1;
}
