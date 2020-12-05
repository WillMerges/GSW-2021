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
// must rely on the VCM library to write to shmem
class TelemetryParser {
public:
    TelemtryParser(); // uses default config_file
    TelemtryParser(std::string config_file); // config file must be same as VCM lib uses
    RetType Parse(); // runs loop until sent stop signal or error (returns failure on error, success on stop)
    void Stop(); // stop the parser
private:
    volatile bool stop;
};

#endif
