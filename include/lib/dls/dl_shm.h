/*******************************************************************************
* Name: dl_shm.h
*
* Purpose: Data Logging Shared Memory
*
* Author: Will Merges
*
* RIT Launch Initiative
*******************************************************************************/
#ifndef DL_SHM_H
#define DL_SHM_H

#include "lib/shm/shm.h"
#include "common/types.h"
#include <string>

using namespace shm;

namespace dls {

    // stores a single boolean that says whether logging is enabled or disabled
    class DlShm {
    public:
        // constructor
        DlShm();

        // destructor
        ~DlShm();

        // initialize
        RetType init();

        // attach to shared memory
        RetType attach();

        // detach from shared memory
        RetType detach();

        // create shared memory
        // NOTE: initializes logging to enabled
        RetType create();

        // destroy shared memory
        RetType destroy();

        // determine if logging to disk is enabled currently
        RetType logging_enabled(bool* enabled);

        // set if logging is enabled or disabled
        RetType set_logging(bool enabled);

    private:
        std::string key_filename;
        static const int key_id = 0x32; // some unique integer

        Shm* shm;
    };
}

#endif
