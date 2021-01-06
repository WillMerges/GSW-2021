#include <stdio.h>
#include <csignal>
#include "lib/ldms/ldms.h"
#include "lib/dls/dls.h"
#include "common/types.h"
#include <csignal>

using namespace ldms;
using namespace dls;

TelemetryParser* tp = NULL;

void sighandler(int signum) {
    MsgLogger logger("DECOM", "");
    logger.log_message("decom killed, cleaning up resources");

    if(tp) {
        delete tp;
    }

    exit(signum);
}

int main() {
    MsgLogger logger("DECOM");
    logger.log_message("starting decom");

    // can't catch sigkill or sigstop though
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGFPE, sighandler);
    signal(SIGABRT, sighandler);

    tp = ldms::create_default_parser();
    if(!tp) {
        printf("Unable to create parser\n");
        return -1;
    }
    tp->Parse();
    return 1;
}
