#include <iostream>
#include <string>
#include "lib/vcm/vcm.h"

using namespace vcm;

int main() {
    VCM vcm;

    for (auto& it : vcm.measurements) {
        std::cout << it << '\n';
    }
}
