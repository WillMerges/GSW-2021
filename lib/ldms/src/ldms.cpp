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

using namespace ldms;
using namespace vcm;
using namespace dls;
using namespace shm;

RetType create_parser_from_VCM(VCM* vcm, TelemetryParser** parser) {
    MsgLogger logger("ldms.cpp", "create_parser_from_VCM");
    switch(vcm->protocol) {
        case UDP:
            *parser = new UDPParser(vcm);
            return SUCCESS;
        default:
            return FAILURE;
    }
}

RetType create_parser(std::string config_file, TelemetryParser** parser) {
    MsgLogger logger("ldms.cpp", "create_parser");
    RetType ret;
    try {
        VCM* vcm = new VCM(config_file);
        ret = create_parser_from_VCM(vcm, parser);
    } catch(std::runtime_error& err) {
        logger.log_message("Cannot create telemetry parser, error in VCM file");
        return FAILURE;
    }
    return ret;
}

RetType create_parser(TelemetryParser** parser) {
    MsgLogger logger("ldms.cpp", "create_parser");
    RetType ret;
    try {
        VCM* vcm = new VCM(); // default VCM
        ret = create_parser_from_VCM(vcm, parser);
    } catch (std::runtime_error& err) {
        logger.log_message("Cannot create telemtry parser, error in VCM file");
        return FAILURE;
    }
    return ret;
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
       throw new std::runtime_error("socket createion failed");
   }

   memset(&servaddr, 0, sizeof(servaddr));

   servaddr.sin_family = AF_INET;
   servaddr.sin_port = htons(vcm->port);
   servaddr.sin_addr.s_addr = INADDR_ANY;

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

    int n;
    while(!stop) {
        n = recvfrom(sockfd, buffer, vcm->packet_size,
                MSG_WAITALL, (struct sockaddr *) &servaddr, NULL);
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
