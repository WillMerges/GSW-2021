#include "lib/dls/dls.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

using namespace dls;

int main() {
    MsgLogger logger;
    while(1) {
        logger.log_message("test");
    }
}
