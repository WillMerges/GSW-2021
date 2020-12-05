#include "lib/dls/dls.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

using namespace dls;

int main(int argc, char* argv[]) {
    MsgLogger logger;
    RetType ret = logger.Open();
    if(ret != SUCCESS) {
        printf("err opening\n");
        exit(-1);
    }
    while(1) {
        logger.log_message("test");
    }
}
