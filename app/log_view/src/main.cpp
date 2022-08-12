#include <sys/inotify.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <unistd.h>
#include <string>
#include "lib/dls/dls.h"

using namespace dls;

int main() {
    MsgLogger logger("LOG_VIEW", "main");

    char* env = getenv("GSW_HOME");
    if(env == NULL) {
        logger.log_message("GSW_HOME environment variable not set!");
        return FAILURE;
    }

    std::string logfile = env;
    logfile += "/log/system.log";
    unsigned int file_index = 1;

    // create inotify descriptor
    uint8_t inotify_buff[sizeof(struct inotify_event) + NAME_MAX + 1]; // can hold at least one event
    int ifd = inotify_init();

    if(-1 == ifd) {
        printf("failed to create inotify descriptor\n");
        logger.log_message("failed to create inotify descriptor");

        return -1;
    }


    int fd;
    int wd;
    std::string path = logfile;
    char c;
    struct inotify_event* event;
    bool closed;
    std::string line;
    while(1) {
        closed = false;

        // open the file
        fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);

        if(fd == -1) {
            printf("failed to attach file: %s\n", path.c_str());
            logger.log_message("failed to attach log file");

            // return -1;
            break;
        }

        // add inotify watch to the opened file
        wd = inotify_add_watch(ifd, path.c_str(), IN_CLOSE_WRITE | IN_MODIFY);
        if(-1 == wd) {
            printf("failed to add inotify watch\n");
            logger.log_message("failed to add inotify watch");

            return -1;
        }

        while(!closed) {
            // print characters until we can't read anymore
            line = "";
            while(0 < read(fd, (void*)&c, sizeof(char))) {
                // printf("%c", c);

                if(c == '\n') {
                    if(line == "end of log file") {
                        closed = true;
                        break;
                    } else {
                        printf("%s\n", line.c_str());
                        line = "";
                    }
                } else {
                    line += c;
                }
            }
            fflush(stdout);

            // don't block if we already reached the end of a log file
            if(closed) {
                break;
            }

            if(errno != SUCCESS && errno != EAGAIN && errno != EWOULDBLOCK) {
                // some unexpected error
                perror("unexpected read error");
                logger.log_message("unexpected read error");

                return -1;
            }

            // wait for the file to change or close
            ssize_t num = read(ifd, (void*)inotify_buff, sizeof(struct inotify_event) + NAME_MAX + 1);
            
            if(-1 == num) {
                printf("inotify read failed\n");
                logger.log_message("inotify read failed");

                return -1;
            }

            while(num) {
                event = (struct inotify_event*)inotify_buff;
                // if(event->mask & IN_CLOSE_WRITE) { // TODO this seems to be always missed, gets handled by "end of log file" message anyways
                //     // file is closed, need to open a new log file
                //     closed = true;
                //
                //     // printf("file closed\n");
                //     // fflush(stdout);
                // }

                if(event->mask & IN_MODIFY) {
                    // we have characters to write

                    // printf("file modified\n");
                    // fflush(stdout);

                    break;
                }

                num -= (sizeof(struct inotify_event) + event->len);
                event = (struct inotify_event*)((uint8_t*)event + (sizeof(struct inotify_event) + event->len));
            }
        }

        // remove inotify watch
        if(-1 == inotify_rm_watch(ifd, wd)) {
            printf("failed to remove inotify watch\n");
            logger.log_message("failed to remove inotify watch");

            return -1;
        }

        // setup the next file
        path = logfile + std::to_string(file_index);
        file_index++;
    }
}
