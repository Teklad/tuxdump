#ifndef  __TPROCESS_REGION_H__
#define  __TPROCESS_REGION_H__
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace TProcess {

class Region {
    public:
        uintptr_t Find(const char* pattern, size_t offset);
        uintptr_t GetCallAddress(uintptr_t addr);
        uintptr_t GetAbsoluteAddress(uintptr_t addr, size_t offset, size_t extra);
    private:
        size_t ReadAddress(uintptr_t addr, void* result, size_t len);
    public:
        int pid;
        char pathName[FILENAME_MAX];
        uintptr_t start;
        uintptr_t end;
};

}

#endif //__TPROCESS_REGION_H__
