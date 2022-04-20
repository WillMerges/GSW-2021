#include <stdio.h>
#include "lib/nm/nm.h"
#include "lib/shm/shm.h"
#include "lib/dls/dls.h"
#include "lib/vcm/vcm.h"
#include "lib/telemetry/TelemetryShm.h"
#include "common/types.h"
#include <csignal>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
*   This executable runs the "decom master process"
*   The master process reads the VCM file, and for each telemetry packet described
*   in the VCM file, forks itself into a child decom process. Each child process
*   listens to the specified port of the telemetry packet and if it writes each
*   packet it receives to that telemetry packet's block in shared memory.
*
*   When the decom master process is killed, it kills each child process as well.
*   If a child process dies unexpectedly, either due to error or being manually
*   sent a kill signal, the master process will report it through the message log.
*
*   Run as: ./decom [config file path]
*   If no VCM config file path is specified, the default location is used
*/


using namespace dls;
using namespace vcm;
using namespace nm;
using namespace shm;


VCM* veh = NULL;

NetworkReceiver* net = NULL;
std::string decom_id = "DECOM[master]"; // id for each decom proc spawned

bool child_proc = false;
std::vector<pid_t> pids;

bool ignore_kill = false;

bool killed = false;
int received_sig = 0;


void sighandler(int signum) {
    MsgLogger logger(decom_id.c_str(), "sighandler");

    if(ignore_kill) {
        logger.log_message("attempt to kill ignored");
        return; // ignored
    }


    if(!child_proc) { // if we're the main process, kill all the children :(
        logger.log_message("reaping children sub-processes");
        size_t i = 0;
        for(pid_t pid : pids) {
            if(-1 == kill(pid, SIGTERM)) {
                logger.log_message("failed to kill DECOM[" + std::to_string(i) +
                                    "] child with PID: " + std::to_string(pid));
                // ignore and kill the other children
            } else {
                logger.log_message("killed DECOM[" + std::to_string(i) +
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

// clean up memory of a child
void child_cleanup() {
    MsgLogger logger(decom_id.c_str(), "child_cleanup");

    logger.log_message("killed, cleaning up resources");
    if(net) { // it's possible we get killed before we can create our network manager
        delete net; // this also calls close
    }
}

// main logic for each sub process to run
// only exit if something bad happens
void execute(size_t packet_id, packet_info_t* packet) {
    decom_id = "DECOM[" + std::to_string(packet_id) + "]";

    // create message logger
    MsgLogger logger(decom_id.c_str(), "execute");

    // set packet name to use for logging messages and network manager name
    std::string packet_name = veh->device + "(" + std::to_string(packet_id) + ")";

    // create/init the network receiver
    // NOTE: on linux kernel 4.15 (tested on) if in a recvfrom call (like in rx() function) the default action is SA_RESTART
    // this means if we catch signal while blocked in recvfrom, we'll never wake up
    // we can either use sigaction instead of signal, or just set a timeout so we wake up once in a while to check if we got a signal
    // for now we just use a timeout, 1s is reasonable and doesn't kill our performance (hence the 1000ms timeout in NetworkReceiver init function)
    if(veh->get_auto_net(packet->port)) {
        AutoNetworkReceiver* n = new AutoNetworkReceiver();

        if(SUCCESS != n->init(veh, packet->port, veh->multicast_addr, 1000, packet->size)) {
            logger.log_message("failed to initialize auto network receiver");
            delete n;
            return;
        }

        net = n;
    } else {
        net = new NetworkReceiver();

        if(SUCCESS != net->init(packet->port, veh->multicast_addr, 1000, packet->size)) {
            logger.log_message("failed to initialized network receiver");
            delete net;
            return;
        }
    }

    // open shared memory
    TelemetryShm shmem;
    if(shmem.init(veh) == FAILURE) {
        logger.log_message("failed to init telemetry shared memory");
        delete net;
        return;
    }

    // attach to shared memory
    if(shmem.open() == FAILURE) {
        logger.log_message("failed to attach to telemetry shared memory");
        delete net;
        return;
    }

    // create packet logger
    PacketLogger plogger(packet_name);

    // main loop
    ssize_t n = 0;
    while(!killed) {
        // read any incoming message and write it to shared memory
        if((n = net->rx()) > 0) {
            if(n != (ssize_t)packet->size) {
                logger.log_message("Packet size mismatch, " + std::to_string(packet->size) +
                                   " != " + std::to_string(n) + " (received)");
            } else { // only write the packet to shared mem if it's the correct size
                // no need to lock the packet for writing here, telemetry (non-virtual) packets should only have one writer
                if(shmem.write(packet_id, net->rx_buffer) == FAILURE) {
                    logger.log_message("failed to write packet to shared memory");
                    // ignore and continue
                }
            }

            plogger.log_packet((unsigned char*)net->rx_buffer, n); // log the packet either way
        }
    }

    // we got killed and got a signal, making net->rx fail
    child_cleanup();
    exit(received_sig);
}

int main(int argc, char** argv) {
    MsgLogger logger(decom_id.c_str(), "main");
    logger.log_message("starting decom master process");

    // interpret the 1st argument as a config_file location if available
    std::string config_file = "";
    if(argc > 1) {
        config_file = argv[1];
    }

    // can't catch sigkill or sigstop though
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGFPE, sighandler);
    signal(SIGABRT, sighandler);

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

    logger.log_message("starting decom sub-processes");

    // according to packets in vcm, spawn a bunch of processes
    // we don't want to killed in the process of making these children so we ignore kill signals
    ignore_kill = true;

    pid_t pid;
    size_t i = 0;
    for(packet_info_t* packet : veh->packets) {
        if(packet->is_virtual) {
            // this is a virtual telemetry packet, so it has no network input
            i++;
            continue;
        }

        pid = fork();
        if(pid == -1) {
            logger.log_message("failed to start decom sub-process " + std::to_string(i));
        } else if(pid == 0) { // we are the child
            // if we allowed killing during this step, the child process could be killed before it's able to set the 'child_proc' boolean
            // which would make it try and clean up other children on kill, and we dont want multiple processes trying to kill each other
            child_proc = true;
            ignore_kill = false;
            execute(i, packet);

            return -1; // if a child returns, something bad happened to it and it should exit
        }

        // otherwise we're the parent, keep going
        logger.log_message("started decom sub-process [" + std::to_string(i) +
                                            "] with PID: " + std::to_string(pid));
        i++;
        pids.push_back(pid);
    }

    ignore_kill = false;

    // monitor children processes in case they die
    while(1) {
        pid = wait(NULL);
        if(pid == -1) { // error
            if(errno == ECHILD) { // no more children left to wait for
                logger.log_message("all decom sub-process children have died, exiting unexpectedly");
                return -1;
            }
        }

        logger.log_message("decom sub-process child with PID: " + std::to_string(pid) + " died unexpectedly");
        // continue and keep monitoring other children
    }

    // should never get here
    return -1;
}
