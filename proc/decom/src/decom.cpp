#include <stdio.h>
#include <csignal>
#include "lib/ldms/ldms.h"
#include "lib/dls/dls.h"
#include "common/types.h"
#include <csignal>
#include <string>

using namespace ldms;
using namespace dls;

TelemetryParser* tp = NULL;

void sighandler(int signum) {
    MsgLogger logger("DECOM");
    logger.log_message("decom killed, cleaning up resources");

    if(tp) {
        delete tp;
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

    if(config_file == "") {
        tp = ldms::create_default_parser();
    } else {
        tp = ldms::create_parser(config_file);
    }
    if(!tp) {
        printf("Unable to create parser\n");
        return -1;
    }
    tp->Parse();
    return 1;
}
