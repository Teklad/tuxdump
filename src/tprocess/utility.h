#ifndef __TPROCESS_UTILITY_H__
#define __TPROCESS_UTILITY_H__
#include <cstdint>
#include <vector>

namespace TProcess {
    struct binary_data_t {
        bool mask;
        uint8_t byte;
    };
    typedef std::vector<binary_data_t> binary_t;
    binary_t HexToBinary(const char* hex);
}

#endif
