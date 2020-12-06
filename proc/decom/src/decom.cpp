#include <stdio.h>
#include "lib/ldms/ldms.h"
#include "common/types.h"

using namespace ldms;

int main() {
    TelemetryParser* tp = ldms::create_default_parser();
    if(!tp) {
        printf("Unable to create parser\n");
        return -1;
    }
    tp->Parse();
    return 1;
}
