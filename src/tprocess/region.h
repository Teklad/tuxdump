#ifndef  __TPROCESS_REGION_H__
#define  __TPROCESS_REGION_H__
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace TProcess {

class Region {
    public:
        Region(int iPid, const char* szPathName, uintptr_t nStart, uintptr_t nEnd);
        Region(const Region&) = delete;
        Region& operator=(const Region&) = delete;
        Region& operator=(Region&&) = default;
        Region(Region&&) = default;
        uintptr_t Find(const std::string& pattern, size_t offset);
        uintptr_t GetCallAddress(uintptr_t addr);
        uintptr_t GetAbsoluteAddress(uintptr_t addr, size_t offset, size_t extra);
    private:
        size_t ReadAddress(uintptr_t addr, void* result, size_t len);
        std::vector<uint8_t> m_readBuffer;
    public:
        int pid;
        char pathName[FILENAME_MAX];
        uintptr_t start;
        uintptr_t end;
};

}

#endif //__TPROCESS_REGION_H__
