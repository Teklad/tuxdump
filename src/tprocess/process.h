#ifndef  __TPROCESS_PROCESS_H__
#define  __TPROCESS_PROCESS_H__
#include "memory.h"
#include "region.h"

#include <sys/uio.h>
#include <cstdint>
#include <vector>

namespace TProcess {

class Process : public Memory {
    public:
        Process() = default;
        ~Process() {};
        bool Attach(const char* name, size_t timeout);
        bool ProcessPresent() const;
        bool GetRegion(const char* name, Region& region_out) const;
        bool HasRegion(const char* name) const;
        bool ParseMaps();
    private:
        char m_szProcDir[256];
        void UpdateRegion(const char* name, uintptr_t start, uintptr_t end);
        std::vector<Region> m_regions;
};

}

#endif

