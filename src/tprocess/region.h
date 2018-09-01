#ifndef  __TPROCESS_REGION_H__
#define  __TPROCESS_REGION_H__
#include <cstddef>
#include <cstdint>

#define TPROCESS_MAX_REGION_NAME 256
namespace TProcess {

struct Region {
    public:
        Region();
        Region(const char* name, int pid, uintptr_t start, uintptr_t end);
        uintptr_t Find(const char* pattern, size_t offset);
        uintptr_t GetCallAddress(uintptr_t addr);
        uintptr_t GetAbsoluteAddress(uintptr_t addr, size_t offset, size_t extra);
    private:
        bool ReadAddress(uintptr_t addr, void* result, size_t len);

    public:
        char name[TPROCESS_MAX_REGION_NAME];
        int pid;
        uintptr_t start;
        uintptr_t end;
};

}

#endif //__TPROCESS_REGION_H__
