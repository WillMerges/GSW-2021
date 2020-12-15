#include "lib/dls/dls.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <string.h>
#include <string>
#include <thread>
#include <iostream>
#include <fstream>
#include <sys/time.h>

#define MAX_LINES_PER_FILE 4096

using namespace dls;

bool verbose = false;
struct timeval curr_time;

//std::ofstream fifo;

// from stack overflow https://stackoverflow.com/questions/3056307/how-do-i-use-mqueue-in-a-c-program-on-a-linux-based-system
#define CHECK(x) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
            perror(#x); \
            exit(-1); \
        } \
    } while (0) \

void read_queue(const char* queue_name, const char* outfile_name, bool binary) {
    unsigned int file_index = 0;
    std::string filename = outfile_name;

    mqd_t mq;
    struct mq_attr attr;
    char buffer[MAX_Q_SIZE + 1];

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_Q_SIZE;
    attr.mq_curmsgs = 0;

    mq = mq_open(queue_name, O_CREAT | O_RDONLY, 0644, &attr);
    CHECK((mqd_t)-1 != mq);

    while(1) {
        std::ofstream file;

        if(binary) {
            file.open(filename.c_str(), std::ios::out | std::ios::trunc);
        } else {
            file.open(filename.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        }

        if(!file.is_open()) {
            printf("Failed to open file: %s\n", outfile_name);
            exit(-1);
        }

        gettimeofday(&curr_time, NULL);
        std::string timestamp = "[" + std::to_string(curr_time.tv_sec) + "]";
        file << timestamp << " Starting DLP\n";
        file.flush();

        ssize_t read = -1;
        unsigned int writes = 0;
        while(writes < MAX_LINES_PER_FILE) {
            gettimeofday(&curr_time, NULL);
            std::string timestamp = "[" + std::to_string(curr_time.tv_sec) + "]";

            read = mq_receive(mq, buffer, MAX_Q_SIZE, NULL);
            if(read == -1) {
                printf("Read failed from MQueue: %s\n", queue_name);
            }
            buffer[read] = '\0';
            if(verbose) {
                if(binary) {
                    printf("%s received telemetry packet\n", timestamp.c_str());
                } else {
                    printf("%s\n", buffer);
                }
            }

            if(binary) {
                file << timestamp << buffer;
                //fifo << timestamp << buffer;
            } else {
                file << timestamp << " " << buffer << '\n';
                //fifo << timestamp << " "  << buffer << '\n';
            }
            writes++;

            fflush(stdout);
            file.flush();
        }
        file_index++;
        filename = outfile_name;
        filename += std::to_string(file_index);
    }
}

int main(int argc, char* argv[]) {
    if(argc > 1) {
        if(!strcmp(argv[1], "-v")) {
            printf("Running in verbose mode.\n\n");
            verbose = true;
        }
    }
    char* env = getenv("GSW_HOME");
    std::string gsw_home;
    if(env == NULL) {
        printf("Could not find GSW_HOME environment variable!\n \
                Did you run '. setenv'?\n");
        exit(-1);
    } else {
        size_t size = strlen(env);
        gsw_home.assign(env, size);
    }

    // std::string fifo_file = gsw_home + "/log/system.fifo";
    // mkfifo(fifo_file.c_str(), O_WRONLY | O_NONBLOCK);
    // fifo.open(fifo_file.c_str(), std::ios::out);
    // if(!fifo.is_open()) {
    //     printf("Failed to open fifo: %s", fifo_file.c_str());
    //     exit(-1);
    // }

    std::string msg_file = gsw_home + "/log/system.log";
    std::string tel_file = gsw_home + "/log/telemetry.log";

    std::thread m_thread(read_queue, MESSAGE_MQUEUE_NAME, msg_file.c_str(), false);
    std::thread t_thread(read_queue, TELEMETRY_MQUEUE_NAME, tel_file.c_str(), true);

    m_thread.join();
    t_thread.join();
}
