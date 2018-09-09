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
        bool Attach(const std::string& name);
        bool ProcessPresent() const;
        bool GetRegion(const std::string& name, Region& region_out);
        bool HasRegion(const std::string& name);
        bool ParseMaps();
    private:
        char m_szProcDir[256];
        std::vector<Region> m_regions;
};

}

#endif

