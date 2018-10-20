#ifndef __TUXDUMP_LOGGER_H__
#define __TUXDUMP_LOGGER_H__
#include <fmt/format.h>

class Logger {
    public:
        Logger() = delete;
        template<typename... Targs>
        static inline void Error(const char* fmt, Targs... args);
        template<typename... Targs>
        static inline void Log(const char* fmt, Targs... args);
        template<typename... Targs>
        static inline void Print(const char* fmt, Targs... args);
        template<typename... Targs>
        static inline void Warn(const char* fmt, Targs... args);
        static inline void EOL();
};

template<typename... Targs>
inline void Logger::Error(const char* fmt, Targs... args)
{
    fmt::print(stderr, "[ERR] ");
    fmt::print(stderr, fmt, args...);
    fmt::print(stderr, "\n");
}

template<typename... Targs>
inline void Logger::Log(const char* fmt, Targs... args)
{
    fmt::print(stderr, fmt, args...);
    fmt::print(stderr, "\n");
}

template<typename... Targs>
inline void Logger::Print(const char* fmt, Targs... args)
{
    fmt::print(stdout, fmt, args...);
}

template<typename... Targs>
inline void Logger::Warn(const char* fmt, Targs... args)
{
    fmt::print(stderr, "[WARN] ");
    fmt::print(stderr, fmt, args...);
    fmt::print(stderr, "\n");
}

inline void Logger::EOL()
{
    fmt::print(stderr, "\n");
}

#endif //__TUXDUMP_LOGGER_H__
