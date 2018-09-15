#ifndef  __TPROCESS_PROCESS_H__
#define  __TPROCESS_PROCESS_H__
#include "memory.h"
#include "region.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include <sys/uio.h>

namespace TProcess {

class Process : public Memory {
    public:
        Process() = default;
        ~Process() {};
        bool Attach(const char* name);
        inline bool Attach(const std::string& name) { return Attach(name.c_str()); }
        bool ProcessPresent() const;
        Region& GetRegion(const std::string& name);
        bool HasRegion(const std::string& name);
        bool ParseMaps();
    private:
        char m_szProcDir[64];
        std::vector<Region> m_regions;
};

}

#endif

