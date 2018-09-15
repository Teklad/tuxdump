#include "utility.h"

#include <cctype>
#include <cstdlib>

namespace TProcess {

/**
 * @brief Given a hexadecimal string, converts it to a binary form for use
 *        in pattern scanning.  Wildcards are accepted as ".."
 *
 * @param hex The hexadecimal string
 *
 * @return binary_t std::vector<struct {mask, byte}>
 */
binary_t HexToBinary(const char* hex)
{
    binary_t result;
    for (const char* p = hex; *p != 0; p += 2) {
        if (*p == '.' && *(p + 1) == '.') {
            result.push_back({true, 0});
        } else if (isxdigit(*p) && isxdigit(*(p + 1))) {
            char convert[3] = {*p, *(p + 1), 0};
            uint8_t byte = strtol(convert, nullptr, 16);
            result.push_back({false, byte});
        } else {
            result.clear();
            break;
        }
    }
    return result;
}
}
