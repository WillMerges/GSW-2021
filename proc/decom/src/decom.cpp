#include <stdio.h>
#include <csignal>
#include "lib/ldms/ldms.h"
#include "lib/dls/dls.h"
#include "common/types.h"

using namespace ldms;
using namespace dls;

TelemetryParser* tp;

void sighandler(int signum) {
    MsgLogger logger("DECOM", "");
    logger.log_message("decom killed, cleaning up resources");
    delete tp;

    exit(signum);
}

int main() {
    tp = ldms::create_default_parser();
    if(!tp) {
        printf("Unable to create parser\n");
        return -1;
    }
    tp->Parse();
    return 1;
}
