#ifndef  __TPROCESS_MEMORY_H__
#define  __TPROCESS_MEMORY_H__
#include <cstdint>
#include <string>

#include <sys/uio.h>

namespace TProcess {

class Memory {
    public:
        Memory() = default;
        virtual ~Memory() {};

        /**
         * @brief Gets the PID of the process this memory object is linked to
         *
         * @return Integer containing the PID of the process
         */
        inline int PID() { return this->m_iPid; }
        
        /**
         * @brief Links the memory object to a new PID
         *
         * @param pid The pid to link to
         */
        inline void SetPID(int pid) { this->m_iPid = pid; }

        /**
         * @brief Reads the memory address in the process, returning T if the
         *        read was successful, or a default constructed T otherwise.
         *        Optional success argument for better error checking.
         *
         * @tparam T The type of variable to read
         * @param addr Address to read from the process
         * @param len Number of bytes to read from the process
         * @param success Optional pointer to a boolean type for error handling
         *
         * @return T
         */
        template<typename T>
        inline T Read(uintptr_t addr, size_t len = sizeof(T), bool* success = nullptr)
        {
            T value;
            struct iovec local = {&value, len};
            struct iovec remote = {reinterpret_cast<void*>(addr), len};
            ssize_t readLength = process_vm_readv(m_iPid, &local, 1, &remote, 1, 0);
            if (success) {
                *success = (readLength == static_cast<ssize_t>(len));
            }
            return value;
        }

        template<typename T>
        inline T Read(uintptr_t addr, bool* success)
        {
            return this->Read<T>(addr, sizeof(T), success);
        }

        inline bool ReadToBuffer(uintptr_t addr, void* buffer, size_t len)
        {
            struct iovec local = {buffer, len};
            struct iovec remote = {reinterpret_cast<void*>(addr), len};
            ssize_t readLength = process_vm_readv(m_iPid, &local, 1, &remote, 1, 0);
            return (readLength == static_cast<ssize_t>(len));
        }

        /**
         * @brief Writes the given value to the memory address in the process
         *
         * @tparam T the type of value to write
         * @param addr Address to write in the process
         * @param value Value to write to the address
         * @param len Number of bytes to write
         *
         * @return true on success, otherwise false
         */
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

    protected:
        static int m_iPid;
};

/**
 * @brief Specialization of TProcess::Memory::Read for handling strings
 *
 * @param addr Address to read from the process
 * @param len Number of bytes to read from the process
 * @param success Optional pointer to a boolean type for better error handling
 *
 * @return std::string containing the string from memory
 */
template<>
inline std::string Memory::Read<std::string>(uintptr_t addr, size_t len, bool* success) {
    std::string result;
    char* buffer = new char[len];
    struct iovec local = {buffer, len};
    struct iovec remote = {reinterpret_cast<void*>(addr), len};
    ssize_t readLen = process_vm_readv(m_iPid, &local, 1, &remote, 1, 0);
    if (success) {
        *success = (readLen == static_cast<ssize_t>(len));
    }
    result = buffer;
    delete[] buffer;
    return result;
}


}

#endif //__TPROCESS_MEMORY_H__
