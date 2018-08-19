#include "config.h"
#include "memory.h"

#include <getopt.h>
#include <cstdio>

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

void PrintOffset(const std::string& module, const std::string& name, uintptr_t offset)
{
    printf("%-30s %-20s %#6lx\n", module.c_str(), name.c_str(), offset);
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

    signatures_t signatureList = cfg.GetSignatures();
    if (signatureList.empty()) {
        fprintf(stderr, "No signatures to process, check your config file for errors.\n");
        return 1;
    }

    printf("%-30s %-20s %-20s\n", "Module", "Name", "Offset");

    TProcess::Memory m(clo.pid);
    TProcess::Memory::Region region;
    for (auto&& s : signatureList) {
        if (s.module == region.name || m.GetRegion(s.module, region)) {
            uintptr_t addr = region.Find(m, s.pattern, s.offset);
            addr = m.GetCallAddress(addr);
            if (addr == 0) {
                PrintOffset(s.module, s.name, 0);
            } else {
                PrintOffset(s.module, s.name, addr + s.extra - region.start);
            }
        }
    }

    return 0;
}

