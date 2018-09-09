#include "config.h"
#include "tprocess/process.h"
#include "tprocess/region.h"
#include "defines.h"
#include "netvars.h"

#include <algorithm>

#include <getopt.h>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>

struct CommandLineOptions {
    std::string cfgFile = "csgo.cfg";
    std::string outputStyle = "";
};

void PrintHelp()
{
    auto PrintOption = [](const char* option, const char* desc) {
        fprintf(stderr, "    %-25s %-10s\n", option, desc);
    };
    fprintf(stderr, "TuxDump - The Linux offset dumper\n");
    fprintf(stderr, "Usage: tuxdump [options]\n");

    PrintOption("--config=, -c <file>", "Alternative configuration file to use");
    PrintOption("--dump-netvars=, -d [style]", "Dumps netvars to a file.  Available styles: raw, cpp");
    PrintOption("--help, -h", "Show this message");

    fprintf(stderr, "\n");
}

bool ParseArguments(int argc, char** argv, CommandLineOptions& clo)
{
    const struct option longOptions[] = {
        {"config", optional_argument, 0, 'c'},
        {"dump-netvars", optional_argument, 0, 'd'}, 
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int optionIndex, c;

    while (true) {
        c = getopt_long(argc, argv, "c::d::h", longOptions, &optionIndex);
        if (c == -1) {
            break;
        }

        switch(c) {
            case 0:
                break;
            case 'c':
                if (optarg != NULL) {
                    clo.cfgFile = optarg;
                }
                break;
            case 'd':
                if (optarg != NULL) {
                    clo.outputStyle = optarg;
                } else {
                    clo.outputStyle = "raw";
                }
                break;
            default:
                return false;
        }
    }

    if (optind < argc) {
        return false;
    }

    return true;
}

inline void PrintOffset(const std::string& module, const std::string& name, uintptr_t offset, const std::string& comment = "")
{
    printf("%-30s %-30s %#-16lx %-20s\n", module.c_str(), name.c_str(), offset, comment.c_str());
}

uintptr_t GetNVOffset(TProcess::Process& m)
{
    TProcess::Region region;
    m.GetRegion("client_panorama_client.so", region);

    uintptr_t ccsp = region.Find("48....085b415c415d5dc3....55488d", 26);
    ccsp = region.GetCallAddress(ccsp);

    return ccsp;
}

int main(int argc, char *argv[])
{

    CommandLineOptions clo;
    TConfig cfg;
    if (!ParseArguments(argc, argv, clo)) {
        PrintHelp();
        return 1;
    }

    if (getuid() != 0) {
        fprintf(stderr, "This process requires root priveleges to work properly.\n");
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
    printf("Process: %-10s\n", cfg.GetProcessName().c_str());
    printf("Version: %-10s\n\n", cfg.GetVersion().c_str());
    printf("==================== Signatures ====================\n");
    printf("%-30s %-30s %-16s %-20s\n", "Module", "Name", "Offset", "Comment");

    TProcess::Process m;
    TProcess::Region region;

    if (!m.Attach(cfg.GetProcessName())) {
        fprintf(stderr, "Failed to attach to process '%s', please make sure it is running.\n",
                cfg.GetProcessName().c_str());
        return 1;
    }

    m.ParseMaps();

    for (auto&& s : signatureList) {
        if (m.GetRegion(s.module, region)) {
            if (s.relative) {
                uintptr_t addr = 0;
                for (size_t i = 0; i < s.offset.size(); ++i) {
                    if (i == 0) {
                        addr = region.Find(s.pattern.c_str(), s.offset[i]);
                        addr = region.GetCallAddress(addr);
                    } else {
                        addr = m.ReadMemory<uintptr_t>(addr + s.offset[i]);
                    }
                }
                if (addr == 0) {
                    PrintOffset(s.module, s.name, 0);
                } else {
                    PrintOffset(s.module, s.name, addr + s.extra - region.start, s.comment);
                }
            } else {
                uintptr_t addr = region.Find(s.pattern.c_str(), s.offset[0]);
                int offset = m.ReadMemory<int>(addr);
                if (offset == 0) {
                    PrintOffset(s.module, s.name, 0);
                } else {
                    PrintOffset(s.module, s.name, offset, s.comment);
                }
            }
        }
    }

    printf("\n");

    if (!clo.outputStyle.empty()) {
        std::transform(clo.outputStyle.begin(), clo.outputStyle.end(), clo.outputStyle.begin(),
                ::tolower);
        NetVarOutputStyle style = NetVarOutputStyle::Raw;
        if (clo.outputStyle == "cpp") {
            style = NetVarOutputStyle::CPlusPlus;
        }

        NetVarManager nvmgr(GetNVOffset(m));
        nvmgr.Dump(style);
    }
    return 0;
}

