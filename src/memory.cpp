#include "memory.h"

#include <limits.h>

#include <cstring>
#include <vector>

namespace TProcess {

/**
 * @brief Typedef for commonly used binary vector
 */
typedef std::vector<std::pair<bool, uint8_t>> binary_t;

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
        memset(hltbl, 256, sizeof(hltbl));
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
 * @return std::vector<bool, uint8_t> (i.e. wildcard,byte)
 */
static binary_t Hex2Bin(const std::string& src)
{
    const uint8_t* hl = HexTable();

    if ((src.size() % 2) != 0) {
        return {};
    }

    binary_t bin;
    for (size_t i = 0; i < src.size(); i += 2) {
        if (src[i] == '.') {
            bin.push_back({true, 0});
        } else {
            bin.push_back({false, (hl[(uint8_t)src[i]] << 4) | hl[(uint8_t)src[i + 1]]});
        }
    }
    return bin;
}


/**
 * @brief Constructs a TProcess::Memory object with the specified PID as the
 *        target.  Attempts to parse /proc/$pid/maps upon construction.
 *
 * @param pid Target process PID
 */
Memory::Memory(int pid) : m_iPid(pid) {
    ParseMaps();
}

/**
 * @brief Given an address, skips [offset] number of opcodes then reads
 *        the destination, returning addr + code + extra.
 *
 * @param addr Original address to read
 * @param offset Number of opcodes to skip prior to read
 * @param extra Bytes to append after the read
 *
 * @return uintptr_t containing the updated address
 */
uintptr_t Memory::GetAbsoluteAddress(uintptr_t addr, size_t offset, size_t extra)
{
    uint32_t code = 0;
    if (this->Read(addr + offset, code, sizeof(uint32_t))) {
        return addr + code + extra;
    }
    return 0;
}

/**
 * @brief Shortcut function which calls GetAbsoluteAddress, skipping one
 *        opcode.  See Memory::GetAbsoluteAddress for more information.
 *
 * @param addr Original address to read
 *
 * @return uintptr_t containing the updated address
 */
uintptr_t Memory::GetCallAddress(uintptr_t addr)
{
    return GetAbsoluteAddress(addr, 1, 5);
}


/**
 * @brief Given a name, looks up known memory regions and returns a struct
 *        containing the region's offsets and short name.
 *
 * @param name Short name of the region (i.e. client.so)
 * @param region_out Region object to modify
 *
 * @return bool: true if the region exists, otherwise false
 */
bool Memory::GetRegion(const std::string& name, Memory::Region& region_out)
{
    for (auto&& r : m_regions) {
        if (r.name == name) {
            region_out = r;
            return true;
        }
    }
    return false;
}

/**
 * @brief Given a region name, tries to update the internal regions vector
 *        with updated offsets.  If a match exists but the memory is not
 *        contiguous, this will create a new entry to prevent overlapping.
 *
 * @param name Short name of the region to update
 * @param start Start address of the region
 * @param end   End address of the region
 */
void Memory::UpdateRegion(const std::string& name, uintptr_t start, uintptr_t end)
{
    for (auto&& r : m_regions) {
        if (r.name == name) {
            if (start == r.end) {
                r.end = end;
            } else {
                continue;
            }
            if (start < r.start) {
                r.start = start;
            }
            return;
        }
    }
    m_regions.push_back({name, start, end});
}


/**
 * @brief Parses /proc/$pid/maps and updates the internal regions vector
 *        with new offsets.  If the region is not named, it is skipped.
 *
 * @return bool: true if the file was opened successfully, false otherwise
 */
bool Memory::ParseMaps()
{
    constexpr size_t bufSize = 0x1000;
    char maps_path[PATH_MAX];
    snprintf(maps_path, PATH_MAX, "/proc/%i/maps", m_iPid);
    FILE* fp = fopen(maps_path, "r");
    if (fp) {
        char line[bufSize];
        while (fgets(line, sizeof(line), fp)) {
            line[strlen(line) - 1] = 0;
            char* sep = strchr(line, '-');
            char* name = strrchr(line, '/');
            if (sep != NULL) {
                uintptr_t start = strtoul(line, NULL, 16);
                uintptr_t end = strtoul(sep + 1, NULL, 16);
                if (name == NULL) {
                    name = strrchr(line, ' ');
                    if (name == NULL) {
                        continue;
                    }
                }
                name += 1;
                if (strlen(name) > 0) {
                    this->UpdateRegion(name, start, end);
                }
            }
        }
        fclose(fp);
        return true;
    }
    return false;
}

/**
 * @brief Attempts to find a given hex pattern within the memory region
 *        If no matches are found, 0 is returned.  Using '..' in the pattern
 *        indicates an unknown byte/nibble which will be ignored.
 *        Example usage:
 *            region.Find(mem, "48..ddfc", 0);
 *
 *
 * @param m Memory object to use for reading memory
 * @param pattern Hexadecimal based pattern
 * @param offset  Number of bytes to offset the result if the pattern is found
 *
 * @return A memory address beginning at the start of the pattern + offset
 */
uintptr_t Memory::Region::Find(Memory& m, const std::string& pattern, size_t offset)
{
    constexpr size_t bufSize = 0x1000;
    uint8_t buf[bufSize];
    binary_t bin = Hex2Bin(pattern.c_str());
    const size_t binSize = bin.size();
    if (bin.empty()) {
        return 0;
    }

    size_t chunk = 0;
    size_t total = this->end - this->start;
    while (total > 0) {
        size_t readSize = (total < bufSize) ? total : bufSize;
        uintptr_t readAddr = this->start + (bufSize * chunk);
        memset(buf, 0, bufSize);
        if (m.Read(readAddr, buf, readSize)) {
            for (uintptr_t b = 0; b < readSize; ++b) {
                size_t match = 0;
                while (bin[match].first || bin[match].second == buf[b + match]) {
                    match++;
                    if (match == binSize) {
                        return readAddr + b + offset;
                    }
                }
            }
        }
        total -= readSize;
        chunk++;
    }

    return 0;
}

};
