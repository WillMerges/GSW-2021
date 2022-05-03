// forwards packets from shared mem. to InfluxDB using UDP line protocol
// run as ./fwd_influx [-f config_file] [-r rate in HZ]
// if config file not specified with -f option, uses the default location
// if a rate is specified with the -f option, the forwards will be rate limited
// NOTE: rate limiting is limited to microsecond accuracy
//
// if there is a measurement called "UPTIME" it will be used as a timestamp
// "UPTIME" is expected to be in units of milliseconds
// otherwise the InfluxDB server clock will be used as the timestamp

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <csignal>
#include "lib/vcm/vcm.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/dls/dls.h"
#include "lib/convert/convert.h"
#include "common/types.h"
#include "lib/clock/clock.h"
#include "common/net.h"

// TODO
// remove printfs? or add verbose mode

using namespace vcm;
using namespace shm;
using namespace dls;
using namespace convert;
using namespace countdown_clock;

#define INFLUXDB_UDP_PORT 8089
// #define INFLUXDB_ADDR "127.0.0.1"
#define INFLUXDB_HOST "influx.local"

#define MILLISEC_PER_SEC     1000
#define MICROSEC_PER_SEC     1000000
#define NANOSEC_PER_MILLISEC 1000000

#define CLOCK_PERIOD 100 // send clock update every 100 ms

int sockfd;
struct sockaddr_in servaddr;
unsigned char sock_open = 0;
bool killed = false;
TelemetryViewer tlm;
VCM* veh;

// signal handler
void sighandler(int) {
    if(sock_open) {
        close(sockfd);
    }

    killed = true;

    tlm.sighandler();
}

// thread that sends clock info once per CLOCK_PERIOD to InfluxDB
void* clock_thread(void*) {
    MsgLogger logger("DB_FWD", "clock_thread");

    CountdownClock cl;

    if(FAILURE == cl.init()) {
        logger.log_message("failed to initialize countdown clock");
        printf("failed to initialize countdown clock\n");
        exit(-1);
    }

    if(FAILURE == cl.open()) {
        logger.log_message("failed to open countdown clock");
        printf("failed to open countdown clock\n");
        exit(-1);
    }

    std::string msg;
    std::string val;

    int64_t clock_time;
    int64_t hold_time;
    bool hold_set;
    bool stopped;
    bool holding;

    while(!killed) {
        msg = veh->device;
        msg += " ";

        if(FAILURE != cl.read_time(&clock_time, &stopped, &holding, &hold_time, &hold_set)) {
            cl.to_str(clock_time, &val);
            msg += "CLOCK=\"";
            msg += val;
            msg += "\",";

            if(hold_set) {
                cl.to_str(hold_time, &val);
                msg += "HOLD=\"";
                msg += val;
                msg += "\"";
            } else {
                msg += "HOLD=\"--:--:--.--\"";
            }
        } else {
            logger.log_message("Failed to read clock time");
            printf("Failed to read clock time\n");
        }

        // send the message
        ssize_t sent = -1;
        sent = sendto(sockfd, msg.c_str(), msg.length(), 0,
            (struct sockaddr*)&servaddr, sizeof(servaddr));
        if(sent == -1) {
            logger.log_message("Failed to send UDP message");
            printf("Failed to send UDP message\n");
            // continue on
        }

        usleep(CLOCK_PERIOD * MILLISEC_PER_SEC);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    MsgLogger logger("DB_FWD");

    logger.log_message("starting database forwarding");

    std::string config_file = "";
    int rate = 0;

    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "-f")) {
            if(i + 1 > argc) {
                logger.log_message("Must specify a path to the config file after using the -f option");
                printf("Must specify a path to the config file after using the -f option\n");
                return -1;
            } else {
                config_file = argv[++i];
            }
        } else if(!strcmp(argv[i], "-r")) {
            if(i + 1 > argc) {
                logger.log_message("must specify a rate after using the -r option");
                printf("must specify a rate after using the -r option\n");
                return -1;
            } else {
                rate = std::stoi(argv[++i], NULL, 10);
            }
        } else {
            std::string msg = "Invalid argument: ";
            msg += argv[i];
            logger.log_message(msg.c_str());
            printf("Invalid argument: %s\n", argv[i]);
            return -1;
        }
    }

    if(config_file == "") {
        veh = new VCM(); // use default config file
    } else {
        veh = new VCM(config_file); // use specified config file
    }

    if(FAILURE == veh->init()) {
        logger.log_message("failed to initialize VCM");
        printf("failed to initialize VCM\n");
        exit(-1);
    }

    // add signal handlers to close the socket if opened
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGFPE, sighandler);
    signal(SIGABRT, sighandler);

    // set up the UDP socket
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
       logger.log_message("socket creation failed");
       printf("socket creation failed\n");
       return FAILURE;
    }
    sock_open = 1;

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(INFLUXDB_UDP_PORT);
    // servaddr.sin_addr.s_addr = inet_addr(INFLUXDB_ADDR);

    if(FAILURE == get_addr(INFLUXDB_HOST, &servaddr.sin_addr)) {
        logger.log_message("failed to resolve InfluxDB server host name");
        printf("failed to resolve host name: %s\n", INFLUXDB_HOST);
        exit(-1);
    }

    if(FAILURE == tlm.init(veh)) {
        logger.log_message("failed to initialize telemetry viewer");
        printf("failed to initialize telemetry viewer\n");
        exit(-1);
    }

    tlm.add_all();
    tlm.set_update_mode(TelemetryViewer::BLOCKING_UPDATE);

    measurement_info_t* m_info;
    std::string msg;
    std::string val;
    // uint32_t timestamp = 0;
    // unsigned char use_timestamp = 0;

    // calculate the period to sleep for depending on rate limiting
    useconds_t period = 0;
    if(rate > 0) {
        period = MICROSEC_PER_SEC / rate;
    }

    // start the thread to send clock updates
    pthread_t clock_tid;
    pthread_create(&clock_tid, NULL, &clock_thread, NULL);

    // main loop
    while(1) {
        if(killed) {
            exit(0);
        }

        // update telemetry
        if(FAILURE == tlm.update()) {
            logger.log_message("failed to update telemetry");
            printf("failed to update telemetry\n");
            // ignore and continue
            continue;
        }

        // construct the message
        msg = veh->device;
        msg += " ";

        unsigned char first = 1;

        for(std::string meas : veh->measurements) {
            m_info = veh->get_info(meas);

            // skip if this measurement didn't update so we don't send redundant data
            if(!tlm.updated(m_info)) {
                continue;
            }

            /**
            if(meas == "UPTIME") {
                if(SUCCESS == convert_to(veh, m_info, buff, &timestamp)) {
                    use_timestamp = 1;
                }
            }
            **/

            if(!first) {
                msg += ",";
                first = 0;
            }


            if(SUCCESS == tlm.get_str(m_info, &val)) {
                msg += meas;
                if(STRING_TYPE == m_info->type) {
                    msg += "=\"";
                    msg += val;
                    msg += "\"";
                } else {
                    msg += "=";
                    msg += val;
                }
            }
        }

        // TODO this probably didn't work because there's an extra comma at the end of the measurements line
        // ^^ actually it's because I think the NANOSEC_PER_MILLISEC #define was wrong, it used to be (10^6) which is xor not power, it's worth trying this again
        // we can add a timestamp in nano-seconds to the end of the line in Influx line protocol
        //if(use_timestamp) {
        //    uint64_t nanosec_time = timestamp;
        //    msg += std::to_string(nanosec_time * NANOSEC_PER_MILLISEC);
        //}

        // send the message
        ssize_t sent = -1;
        // std::cout << msg << "\n";
        sent = sendto(sockfd, msg.c_str(), msg.length(), 0,
            (struct sockaddr*)&servaddr, sizeof(servaddr));
        if(sent == -1) {
            logger.log_message("Failed to send UDP message");
            printf("Failed to send UDP message\n");
            // continue on
        }

        // sleep depending on rate limiting
        if(period) {
            usleep(period);
        }
    }
}
