#ifndef  __TPROCESS_MEMORY_H__
#define  __TPROCESS_MEMORY_H__

#include <sys/uio.h>
#include <cstdint>

namespace TProcess {

class Memory {
    public:
        Memory() = default;
        ~Memory() {};
        Memory(const Memory&) = delete;
        Memory& operator=(const Memory&) = delete;
        inline int PID() { return this->m_iPid; }
        inline void SetPID(int pid) { this->m_iPid = pid; }

        template<typename T>
        T Read(uintptr_t addr, size_t len = sizeof(T), bool* success = nullptr)
        {
            T value;
            struct iovec local = {&value, len};
            struct iovec remote = {reinterpret_cast<void*>(addr), len};
            ssize_t readLen = process_vm_readv(m_iPid, &local, 1, &remote, 1, 0);
            if (success) {
                *success = (readLen == static_cast<ssize_t>(len));
            }
            return value;
        }

        bool ReadArray(uintptr_t addr, void* buffer, size_t len)
        {
            struct iovec local = {buffer, len};
            struct iovec remote = {reinterpret_cast<void*>(addr), len};
            ssize_t readLen = process_vm_readv(m_iPid, &local, 1, &remote, 1, 0);
            return (readLen == static_cast<ssize_t>(len));
        }

        template<typename T>
        inline bool Write(uintptr_t addr, T value, size_t len = sizeof(T))
        {
            struct iovec local = {&value, len};
            struct iovec remote = {reinterpret_cast<void*>(addr), len};
            ssize_t result = process_vm_writev(m_iPid, &local, 1, &remote, 1, 0);
            return (result == static_cast<ssize_t>(len));
        }

        template<typename T>
        inline bool Write(uintptr_t addr, T& value, size_t len = sizeof(T))
        {
            struct iovec local = {&value, len};
            struct iovec remote = {reinterpret_cast<void*>(addr), len};
            ssize_t result = process_vm_writev(m_iPid, &local, 1, &remote, 1, 0);
            return (result == static_cast<ssize_t>(len));
        }

    private:
        int m_iPid = -1;
};

}

#endif //__TPROCESS_MEMORY_H__
