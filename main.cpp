#include <iostream>
#include "ffstreamer.h"

int main(int argc, char* argv[]) {
    if(argc < 2) {
        std::cerr << "USAGE: "<< argv[0] << " <input> <output>" << std::endl;
        return 1;
    }

    const char* in = argv[1];
    const char* out= argv[2];

    FFstreamer ffs(in, out);
    ffs.init();

    return ffs.runStreaming();
}
