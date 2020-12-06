#include "lib/ldms/ldms.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <exception>
#include "common/types.h"
#include "lib/vcm/vcm.h"
#include "lib/dls/dls.h"
#include "lib/shm/shm.h"

using namespace vcm;
using namespace dls;
using namespace shm;

namespace ldms {

    TelemetryParser* create_parser_from_VCM(VCM* vcm) {
        MsgLogger logger("ldms.cpp", "create_parser_from_VCM");
        switch(vcm->protocol) {
            case UDP:
                return new UDPParser(vcm);
            default:
                return NULL;
        }
    }

    TelemetryParser* create_parser(std::string config_file) {
        MsgLogger logger("ldms.cpp", "create_parser");
        try {
            VCM* vcm = new VCM(config_file);
            return create_parser_from_VCM(vcm);
        } catch(std::runtime_error& err) {
            logger.log_message("Cannot create telemetry parser, error in VCM file");
            return NULL;
        }
    }

    TelemetryParser* create_default_parser() {
        MsgLogger logger("ldms.cpp", "create_parser");
        try {
            VCM* vcm = new VCM(); // default VCM
            return create_parser_from_VCM(vcm);
        } catch (std::runtime_error& err) {
            logger.log_message("Cannot create telemetry parser, error in VCM file");
            return NULL;
        }
    }

    TelemetryParser::TelemetryParser(VCM* vcm) {
        stop = false;
        set_shmem_size(vcm->packet_size);
        attach_to_shm();
        shmem = get_shm_block();
        this->vcm = vcm;
    }

    TelemetryParser::~TelemetryParser() {
        if(vcm) {
            delete vcm;
        }
        detach_from_shm();
    }

    void TelemetryParser::Stop() {
        stop = true;
    }

    UDPParser::UDPParser(VCM* vcm) : TelemetryParser(vcm) {
        MsgLogger logger("UDPParser", "Constructor");

        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
           logger.log_message("socket creation failed");
           throw new std::runtime_error("socket creation failed");
       }

       memset(&servaddr, 0, sizeof(servaddr));

       servaddr.sin_family = AF_INET;
       servaddr.sin_port = htons(vcm->port);
       servaddr.sin_addr.s_addr = htons(INADDR_ANY);

       struct sockaddr_in myaddr;
       myaddr.sin_addr.s_addr = htons(INADDR_ANY);
       myaddr.sin_family = AF_INET;
       myaddr.sin_port = htons(GSW_PORT); // TODO maybe use multiple ports for different devices?
       int rc = bind(sockfd, (struct sockaddr*) &myaddr, sizeof(myaddr));
       if(rc) {
           logger.log_message("socket bind failed");
           throw new std::runtime_error("socket bind failed");
       }

       buffer = new char[vcm->packet_size];
    }

    UDPParser::~UDPParser() {
        if(buffer) {
            delete buffer;
        }
    }

    void UDPParser::Parse() {
        MsgLogger logger("UDPParser", "Parse");
        PacketLogger plogger("UDP(" + std::to_string(vcm->src) +
                             "," + std::to_string(vcm->port) + ")");

        int n = -1;
        socklen_t len = sizeof(servaddr);
        while(!stop) {
            n = recvfrom(sockfd, buffer, vcm->packet_size,
                    MSG_WAITALL, (struct sockaddr *) &servaddr, &len);

            if(n != (int)(vcm->packet_size)) {
                logger.log_message("Packet size mismatch, " +
                                    std::to_string(vcm->packet_size) +
                                    " != " + std::to_string(n) + " (received)");
                memset(shmem, 0, vcm->packet_size);
            } else {
                plogger.log_packet((unsigned char*)buffer, n);
                memcpy(shmem, buffer, n);
            }
        }
    }

    // default
    void TelemetryParser::Parse() {
        while(!stop) {};
    }

}
