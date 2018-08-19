#pragma once

#include <sys/uio.h>

#include <cstdint>
#include <string>
#include <vector>

namespace TProcess {

class Memory {
    public:
        struct Region {
            std::string name;
            uintptr_t start = 0;
            uintptr_t end = 0;
            uintptr_t Find(Memory& memory, const std::string& pattern, size_t offset = 0);
        };

        Memory(int pid);
        ~Memory() = default;
        Memory(const Memory&) = delete;
        Memory& operator=(const Memory&) = delete;
        uintptr_t GetAbsoluteAddress(uintptr_t addr, size_t offset, size_t extra);
        uintptr_t GetCallAddress(uintptr_t addr);
        bool GetRegion(const std::string& name, Memory::Region& region_out);
        bool ParseMaps();


        template<typename T>
        bool Read(uintptr_t addr, T& value, size_t len = sizeof(T))
        {
            struct iovec local = {&value, len};
            struct iovec remote = {reinterpret_cast<void*>(addr), len};
            ssize_t result = process_vm_readv(m_iPid, &local, 1, &remote, 1, 0);
            return (result == static_cast<ssize_t>(len));
        }

        template<typename T>
        bool Write(uintptr_t addr, T value, size_t len = sizeof(T))
        {
            struct iovec local = {&value, len};
            struct iovec remote = {reinterpret_cast<void*>(addr), len};
            ssize_t result = process_vm_writev(m_iPid, &local, 1, &remote, 1, 0);
            return (result == static_cast<ssize_t>(len));
        }
    private:
        void UpdateRegion(const std::string& name, uintptr_t start, uintptr_t end);
        std::vector<Region> m_regions;
        int m_iPid = -1;
};

};
