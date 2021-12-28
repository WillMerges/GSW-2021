#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "lib/vlock/vlock.h"

int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Usage: ./test <lock | unlock>\n");
        return -1;
    }

    std::string arg = argv[1];
    bool lock = false;

    if(arg == "lock") {
        lock = true;
    } else if(arg == "unlock") {
        lock = false;
    } else {
        printf("Usage: ./test <lock | unlock>\n");
        return -1;
    }

    if(SUCCESS != vlock::init()) {
        printf("Failed to init vlock lib\n");
        return -1;
    }

    if(lock) {
        if(SUCCESS != vlock::try_lock(vlock::ENGINE_CONTROLLER)) {
            printf("Lock failed\n");
            return -1;
        }
    } else {
        if(SUCCESS != vlock::unlock(vlock::ENGINE_CONTROLLER)) {
            printf("Unlock failed\n");
            return -1;
        }
    }

    printf("Success\n");
}
