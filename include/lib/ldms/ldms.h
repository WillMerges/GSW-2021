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


// responsible for reading telemtry data and writing it to shared mem
// this should be the only lib that needs to use the shm lib
// TODO not the best name
class TelemetryParser {
public:
    TelemetryParser(); // uses default config_file
    TelemetryParser(std::string config_file); // config file must be same as VCM lib uses
    RetType Parse(); // runs loop until sent stop signal or error (returns failure on error, success on stop)
    void Stop(); // stop the parser
private:
    // maybe store src and port differently
    int src;
    int port;
    volatile bool stop;
};

#endif
