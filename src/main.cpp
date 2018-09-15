#include "config.h"
#include "tprocess/process.h"
#include "tprocess/region.h"
#include "netvars.h"

#include <algorithm>

#include <getopt.h>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>

TProcess::Process process;
struct CommandLineOptions {
    std::string cfgFile = "csgo.cfg";
    std::string outputStyle = "";
    bool dumpTypeInformation = false;
};

static void PrintHelp()
{
    auto PrintOption = [](const char* option, const char* desc) {
        fprintf(stderr, "    %-25s %-10s\n", option, desc);
    };
    fprintf(stderr, "TuxDump - The Linux offset dumper\n");
    fprintf(stderr, "Usage: tuxdump [options]\n");

    PrintOption("--config=, -c <file>", "Alternative configuration file to use");
    PrintOption("--dump-netvars=, -d [style]", "Dumps netvars to a file.  Available styles: raw, cpp");
    PrintOption("--help, -h", "Show this message");
    PrintOption("--types, -t", "Dump type information to netvars");
    fprintf(stderr, "\n");
}

static bool ParseArguments(int argc, char** argv, CommandLineOptions& clo)
{
    const struct option longOptions[] = {
        {"config", optional_argument, 0, 'c'},
        {"dump-netvars", optional_argument, 0, 'd'}, 
        {"help", no_argument, 0, 'h'},
        {"types", no_argument, 0, 't'},
        {0, 0, 0, 0}
    };
    
    int optionIndex, c;

    while (true) {
        c = getopt_long(argc, argv, "c::d::ht", longOptions, &optionIndex);
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
            case 't':
                clo.dumpTypeInformation = true;
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

static inline void PrintOffset(const std::string& module,
        const std::string& name,
        uintptr_t offset,
        const std::string& comment = "")
{
    printf("%-30s %-30s %#-16lx %-20s\n", module.c_str(), name.c_str(), offset, comment.c_str());
}

static void ReadSignature(const SignatureDefinition_t& signature)
{
    try {
        TProcess::Region& region = process.GetRegion(signature.module);
        if (signature.relative) {
            uintptr_t addr = 0;
            addr = region.Find(signature.pattern, signature.offset[0]);
            addr = region.GetCallAddress(addr);

            // Read through any extra offsets
            for (size_t i = 1; i < signature.offset.size(); ++i) {
                addr = process.ReadMemory<uintptr_t>(addr + signature.offset[i]);
            }

            if (addr == 0) {
                PrintOffset(signature.module, signature.name, 0);
            } else {
                PrintOffset(signature.module, signature.name, 
                        addr + signature.extra - region.start,
                        signature.comment);
            }
        } else {
            uintptr_t addr = region.Find(signature.pattern,
                    signature.offset[0]);
            int offset = process.ReadMemory<int>(addr);
            PrintOffset(signature.module, signature.name, 
                    offset, signature.comment);
        }
    }catch (std::out_of_range& e) {
        fprintf(stderr, "%s: Can't find '%s'\n", e.what(),
                signature.module.c_str());
    }
}

static void DumpNetvars(std::string& outputStyle, bool comments)
{
    constexpr char szClientRegion[] = "client_panorama_client.so";
    std::transform(outputStyle.begin(), outputStyle.end(), outputStyle.begin(),
            ::tolower);
    try {
        TProcess::Region& region = process.GetRegion(szClientRegion);
        uintptr_t clientClassHead = region.Find("48....085b415c415d5dc3....55488d", 26);
        clientClassHead = region.GetCallAddress(clientClassHead);
        NetVarOutputStyle style = NetVarOutputStyle::Raw;
        if (outputStyle == "cpp") {
            style = NetVarOutputStyle::CPlusPlus;
        }

        NetVarManager nvmgr(clientClassHead);
        nvmgr.Dump(style, comments);
    } catch (std::out_of_range& e) {
        fprintf(stderr, "%s: Can't find '%s'\n", e.what(),
                szClientRegion);
    }
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

    if (!process.Attach(cfg.GetProcessName())) {
        fprintf(stderr, "Failed to attach to process '%s', please make sure it is running.\n",
                cfg.GetProcessName().c_str());
        return 1;
    }
    process.ParseMaps();

    for (auto&& s : signatureList) {
        ReadSignature(s);
    }

    printf("\n");

    if (!clo.outputStyle.empty()) {
        DumpNetvars(clo.outputStyle, clo.dumpTypeInformation);
    }
    return 0;
}

