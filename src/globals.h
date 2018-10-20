#ifndef  __TUXDUMP_GLOBALS_H__
#define  __TUXDUMP_GLOBALS_H__
#include <tuxproc/process.h>
#include <libconfig.h++>
#include <cstdarg>

enum class OutputFormat {
    CPP,
    Java,
    Raw
};

extern TuxProc::Process g_process;
extern const char* g_fmtOutput;
extern libconfig::Config g_cfg;
extern libconfig::Config g_cfgFormat;

inline void OutputSpace(int depth = 1)
{
    for (int i = 0; i < depth; ++i) {
        printf("    ");
    }
}
#endif
