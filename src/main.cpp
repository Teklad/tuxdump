#include "config.h"
#include "memory.h"
#include "defines.h"

#include <getopt.h>
#include <cstdio>

#ifdef DEBUG_BUILD
#include <cstring>
#include <unistd.h>
#include <map>
#endif

struct CommandLineOptions {
    std::string cfgFile = "csgo.cfg";
    int pid = 0;
};

void PrintHelp()
{
    auto PrintOption = [](const char* option, const char* desc) {
        fprintf(stderr, "    %-25s %-10s\n", option, desc);
    };
    fprintf(stderr, "TuxDump - The Linux offset dumper\n");
    fprintf(stderr, "Usage: tuxdump [options] <pid>\n");

    PrintOption("--config, -c <file>", "Alternative configuration file to use.");
    PrintOption("--help, -h", "Show this message.");

    fprintf(stderr, "\n");
}

bool ParseArguments(int argc, char** argv, CommandLineOptions& clo)
{
    const struct option longOptions[] = {
        {"config", required_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int optionIndex, c;

    while (true) {
        c = getopt_long(argc, argv, "c:h", longOptions, &optionIndex);
        if (c == -1) {
            break;
        }

        switch(c) {
            case 0:
                break;
            case 'c':
                clo.cfgFile = optarg;
                break;
            default:
                return false;
        }
    }

    if (optind < argc) {
        clo.pid = strtoul(argv[optind], NULL, 10);
        if (clo.pid == 0) {
            return false;
        }
    } else {
        return false;
    }

    return true;
}

inline void PrintOffset(const std::string& module, const std::string& name, uintptr_t offset, const std::string& comment = "")
{
    printf("%-30s %-20s %#-16lx %-20s\n", module.c_str(), name.c_str(), offset, comment.c_str());
}

int main(int argc, char *argv[])
{
    CommandLineOptions clo;
    TConfig cfg;
    if (!ParseArguments(argc, argv, clo)) {
        PrintHelp();
        return 1;
    }
    if (!cfg.ReadFile(clo.cfgFile)) {
        return 1;
    }

    const signatures_t signatureList = cfg.GetSignatures();
    if (signatureList.empty()) {
        fprintf(stderr, "No signatures to process, check your config file for errors.\n");
        return 1;
    }

    printf("Using configuration file: %s\n\n", clo.cfgFile.c_str());
    printf("Name: %-10s\n", cfg.GetName().c_str());
    printf("Version: %-10s\n\n", cfg.GetVersion().c_str());
    printf("==================== Signatures ====================\n");
    printf("%-30s %-20s %-16s %-20s\n", "Module", "Name", "Offset", "Comment");

#ifdef DEBUG_BUILD
    std::map<std::string, uintptr_t> offsets;
#endif

    TProcess::Memory m(clo.pid);
    TProcess::Memory::Region region;
    for (auto&& s : signatureList) {
        if (s.module == region.name || m.GetRegion(s.module, region)) {
            uintptr_t addr = region.Find(m, s.pattern, s.offset);
            addr = m.GetCallAddress(addr);
            if (addr == 0) {
                PrintOffset(s.module, s.name, 0);
            } else {
                PrintOffset(s.module, s.name, addr + s.extra - region.start, s.comment);
#ifdef DEBUG_BUILD
                offsets.emplace(s.name, addr + s.extra);
#endif
            }
        }
    }

    return 0;
}

