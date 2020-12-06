#include "lib/shm/shm.h"
#include <stdint.h>
#include <iostream>

using namespace std;
using namespace shm;

int main() {
    set_size(4);
    attach_to_shm();
    char* b = (char*)get_shm_block();
    cout << b << '\n';
    detach_from_shm();
    destroy_shm();
}
