#include "lib/dls/dls.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

using namespace dls;

void read_queue(const char* queue_name) {
    mqd_t mq;
    struct mq_attr attr;
    char buffer[MAX_Q_SIZE + 1];

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_Q_SIZE;
    attr.mq_curmsgs = 0;

    mq = mq_open(queue_name, O_CREAT | O_RDONLY, 0644, &attr);
    while(1) {
        ssize_t read;
        read = mq_receive(mq, buffer, MAX_Q_SIZE, NULL);
        buffer[read] = '\0';
        printf("%s\n", buffer);
    }
}

int main(int argc, char* argv[]) {
    read_queue("/home/will/test_q");
}
