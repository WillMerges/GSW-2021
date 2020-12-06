#include "lib/shm/shm.h"
#include <stdint.h>

using namespace shm;

int main() {
    set_shmem_size(4);
    attach_to_shm();
    char* b = (char*)get_shm_block();
    b[0] = 'a';
    b[1] = 'b';
    b[2] = 'c';
    b[3] = 0;
    detach_from_shm();
}
