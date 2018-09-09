#include "region.h"

#include <cctype>
#include <cstring>
#include <sys/uio.h>

namespace TProcess {
constexpr size_t TPROC_READ_SIZE = 8192;
/**
 * @brief Structure to hold pattern information
 */
struct binary_t {
    uint8_t data[128] = {0};
    bool    mask[128] = {0};
    size_t  size = 0;
};

/**
 * @brief Initializes the hex table if it isn't already.  Otherwise simply
 *        returns the static hex table
 *
 * @return uint8_t array containing valid hex values
 */
static uint8_t* HexTable()
{
    static uint8_t hltbl[255] = {0};
    if (hltbl['1'] == 0) {
        hltbl['0'] = 0;
        hltbl['1'] = 1;
        hltbl['2'] = 2;
        hltbl['3'] = 3;
        hltbl['4'] = 4;
        hltbl['5'] = 5;
        hltbl['6'] = 6;
        hltbl['7'] = 7;
        hltbl['8'] = 8;
        hltbl['9'] = 9;
        hltbl['a'] = 10;
        hltbl['A'] = 10;
        hltbl['b'] = 11;
        hltbl['B'] = 11;
        hltbl['c'] = 12;
        hltbl['C'] = 12;
        hltbl['d'] = 13;
        hltbl['D'] = 13;
        hltbl['e'] = 14;
        hltbl['E'] = 14;
        hltbl['f'] = 15;
        hltbl['F'] = 15;
    }
    return hltbl;
}

/**
 * @brief Given a hexadecimal string, encodes it to raw bytes
 *        into a std::vector<bool, uint8_t> object.
 *
 * @param src Source string to process
 *
 * @return true if conversion succeeded, otherwise false.
 */
static bool Hex2Bin(const char* src, binary_t& bin)
{
    const size_t src_size = strlen(src);
    const uint8_t* hl = HexTable();
    const uint8_t* usrc = (const uint8_t*)src;

    if ((src_size % 2) != 0) {
        return false;
    }

    for (size_t i = 0; i < src_size; i += 2) {
        if (src[i] == '.') {
            bin.data[bin.size] = 0;
            bin.mask[bin.size] = true;
        } else if (isxdigit(src[i])) {
            bin.data[bin.size] = (hl[usrc[i]] << 4) | hl[usrc[i + 1]];
            bin.mask[bin.size] = false;
        } else {
            return false;
        }
        bin.size++;
    }
    bin.mask[bin.size] = false;
    return true;
}

size_t Region::ReadAddress(uintptr_t addr, void* result, size_t len)
{
    struct iovec local = {result, len};
    struct iovec remote = {reinterpret_cast<void*>(addr), len};
    ssize_t readLength = process_vm_readv(pid, &local, 1, &remote, 1, 0);
    return (readLength > 0 ? readLength : 0);
}

uintptr_t Region::GetAbsoluteAddress(uintptr_t addr, size_t offset, size_t extra)
{
    int32_t code;
    if (ReadAddress(addr + offset, &code, sizeof(int32_t))) {
        return addr + code + extra;
    }
    return 0;
}

/**
 * @brief Given an address, tries to skip the opcode byte and
 *        Get the pointed to address
 *
 * @param addr The address to inspect
 *
 * @return Updated address pointing to data (i.e. qword, etc.)
 */
uintptr_t Region::GetCallAddress(uintptr_t addr)
{
    return GetAbsoluteAddress(addr, 1, 5);
}

/**
 * @brief Attempts to find a given hex pattern within the memory region
 *        If no matches are found, 0 is returned.  Using '..' in the pattern
 *        indicates an unknown byte/nibble which will be ignored.
 *        Example usage:
 *            region.Find(mem, "48..ddfc", 0);
 *
 *
 * @param pattern Hexadecimal based pattern
 * @param offset  Number of bytes to offset the result if the pattern is found
 *
 * @return A memory address beginning at the start of the pattern + offset
 */
uintptr_t Region::Find(const std::string& pattern, size_t offset)
{
    uint8_t buf[TPROC_READ_SIZE] = {0};
    binary_t bin;
    if (!Hex2Bin(pattern.c_str(), bin)) {
        return 0;
    }

    size_t readSize = TPROC_READ_SIZE;
    uintptr_t readAddr = start;
    while (readAddr < end) {
        size_t totalRead = ReadAddress(readAddr, buf, readSize);
        for (size_t b = 0; b < totalRead; ++b) {
            size_t match = 0;
            while (bin.data[match] == buf[b + match]) {
                do {match++;} while (bin.mask[match]);
                if (match == bin.size) {
                    return readAddr + b + offset;
                }
            }
        }
        readAddr += TPROC_READ_SIZE;
        if (readSize > end - readAddr) {
            readSize = end - readAddr;
        }
    }
    return 0;
}

}
