#include "region.h"
#include "utility.h"

#include <cstring>

#include <sys/uio.h>

namespace TProcess {

Region::Region(int iPid, const char* szPathName, uintptr_t nStart, uintptr_t nEnd)
{
    pid = iPid;
    strcpy(pathName, szPathName);
    start = nStart;
    end = nEnd;
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
    binary_t bin = TProcess::HexToBinary(pattern.c_str());
    const size_t binSize = bin.size();
    if (binSize == 0) {
        return 0;
    }

    m_readBuffer.resize(end - start);
    size_t readSize = ReadAddress(start, m_readBuffer.data(), end - start);
    for (size_t i = 0; i < readSize; ++i) {
        bool found = true;
        for (size_t match = 0; match < binSize; ++match) {
            found = bin[match].mask || bin[match].byte == m_readBuffer[match + i];
            if (!found) {
                break;
            }
        }
        if (found) {
            return start + i + offset;
        }
    }
    return 0;
}

}
