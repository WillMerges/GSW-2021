#include "lib/dls/dls.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <unistd.h>
#include <string>

using namespace dls;

int main() {
    MsgLogger logger;

    std::string i_str;
    int i = 0;
    while(1) {
        i_str = std::to_string(i);
        i++;

        logger.log_message(i_str.c_str());
        usleep(10000);
    }
}
