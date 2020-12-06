/********************************************************************
*  Name: ldms.h
*
*  Purpose: Header for launch data management system source.
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#ifndef LDMS_H
#define LDMS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include "common/types.h"
#include "lib/vcm/vcm.h"

namespace ldms {
    // responsible for reading telemtry data and writing it to shared mem
    // this should be the only lib that needs to use the shm lib
    // TODO not the best name
    class TelemetryParser {
    public:
        TelemetryParser(vcm::VCM* vcm);
        ~TelemetryParser();
        virtual void Parse(); // runs loop until sent stop signal or error (returns failure on error, success on stop)
        void Stop(); // stop the parser
    protected:
        volatile bool stop; // in case this is run in a thread we don't want it cached
        vcm::VCM* vcm;
        void* shmem;
    };

    class UDPParser : public TelemetryParser {
    public:
        UDPParser(vcm::VCM* vcm);
        ~UDPParser();
        void Parse();
    private:
        struct sockaddr_in servaddr;
        int sockfd;
        char* buffer;
    };
    // reads the protocol and size from a config file and sets the correct parser for it
    TelemetryParser* create_parser(std::string config_file);
    TelemetryParser* create_default_parser();
}

#endif
