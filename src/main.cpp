#include "tools/tools.h"
#include "globals.h"
#include "logger.h"

#include <cstdio>
#include <cstdlib>

#include <strings.h>
#include <unistd.h>

TuxProc::Process g_process;
libconfig::Config g_cfg;
libconfig::Config g_cfgFormat;
const char* g_fmtOutput;

constexpr const char validTools[][20] = {
    "classids",
    "netvars",
    "signatures"
};

static bool ReadSignatureConfig(const char* configFile)
{
    try {
        g_cfg.readFile(configFile);
    } catch (const libconfig::FileIOException& fioex) {
        return false;
    } catch (const libconfig::ParseException& pex) {
        Logger::Error("Parse exception: {}\nFile: {}:{}\n", 
                pex.getError(), configFile, pex.getLine());
        return false;
    }
    // Validate config structure
    try {
        libconfig::Setting& signatures = g_cfg.lookup("signatures");
        for (libconfig::Setting& entry : signatures) {
            entry.lookup("pattern");
            entry.lookup("region");
            entry.lookup("offset");
            entry.lookup("extra");
            entry.lookup("relative");
        }
        signatures.lookup("dwGetAllClasses");
        signatures.lookup("dwGetAllClasses.offset");
        signatures.lookup("dwGetAllClasses.pattern");
    } catch (const libconfig::SettingNotFoundException& snfex) {
        Logger::Error("{}: {}", snfex.what(), snfex.getPath());
        return false;
    }
    return true;
}

static bool ReadFormatConfig(const char* configFile)
{
    try {
        g_cfgFormat.readFile(configFile);
    } catch (const libconfig::FileIOException& fioex) {
        return false;
    } catch (const libconfig::ParseException& pex) {
        Logger::Error("Parse exception: {}\nFile: {}:{}\n", 
                pex.getError(), configFile, pex.getLine());
        return false;
    }

    try {
        libconfig::Setting& formats = g_cfgFormat.lookup("formats");
        for (libconfig::Setting& entry : formats) {
            entry.lookup("variable_offset");
            entry.lookup("netvars.table_start");
            entry.lookup("netvars.table_end");
        }
    } catch (const libconfig::SettingNotFoundException& snfex) {
        Logger::Error("{}: {}", snfex.what(), snfex.getPath());
        return false;
    }
    return true;
}

static void RunTool(const char* cmdTool)
{
    if (!strcasecmp(cmdTool, "classids")) {
        //run tool classids
    } else if (!strcasecmp(cmdTool, "netvars")) {
        Tools::DumpNetvars();
    } else if (!strcasecmp(cmdTool, "signatures")) {
        Tools::DumpSignatures();
    }
}

static bool CheckTool(const char* cmdTool)
{
    for (size_t i = 0; i < sizeof(validTools)/sizeof(validTools[0]); ++i) {
        if (!strcasecmp(validTools[i], cmdTool)) {
            return true;
        }
    }
    return false;
}

static bool CheckFormat(const char* cmdFormat)
{
    libconfig::Setting& formats = g_cfgFormat.getRoot().lookup("formats");
    for (libconfig::Setting& entry : formats) {
        if (!strcasecmp(entry.getName(), cmdFormat)) {
            return true;
        }
    }
    return false;
}

static void PrintOption(const char* name, const char* desc = nullptr)
{
    if (desc) {
        Logger::Log("    {:<15} {:<15}", name, desc);
    } else {
        Logger::Log("    {:<15}", name);
    }
}

static void PrintHelpFormats()
{
    Logger::Log("Available Formats:");
    libconfig::Setting& formats = g_cfgFormat.lookup("formats");
    for (libconfig::Setting& entry : formats) {
        PrintOption(entry.getName());
    }
    Logger::EOL();
}

static void PrintHelpTools()
{
    Logger::Log("Available Tools:");
    PrintOption("classids", "enumerated list of classids");
    PrintOption("netvars", "netvar offsets");
    PrintOption("signatures", "memory addresses defined in config");
    Logger::EOL();
}

static void PrintHelpOptions()
{
    Logger::Log("Options:");
    PrintOption("-c[filename]", "config file to use");
    PrintOption("-f[format]", "language formatting");
    PrintOption("-h", "this message");
    PrintOption("-p[process]", "name of process to attach");
    Logger::EOL();
}

static void PrintHelpAll()
{
    Logger::Log("Usage: {} [options] <tool>", PROJECT_NAME);
    Logger::Log("{} dumps offsets for the specified executable into different", PROJECT_NAME);
    Logger::Log("language formats for use in memory manipulation.\n");
    Logger::Log("Examples:");
    Logger::Log("    {} -fraw -pcsgo_linux64 -ccsgo.cfg signatures (Default)", PROJECT_NAME);
    Logger::Log("    {} -fcpp signatures", PROJECT_NAME);
    Logger::Log("    {} -fjava netvars\n", PROJECT_NAME);
    PrintHelpOptions();
    PrintHelpFormats();
    PrintHelpTools();
}

int main(int argc, char* argv[])
{
    const char* cmdConfig = "csgo.cfg";
    const char* cmdFormat = "raw";
    const char* cmdProcess = "csgo_linux64";
    const char* cmdTool = "signatures";

    if (!ReadFormatConfig("formats.cfg")) {
        Logger::Error("Failed to read format config file \"{}\"", "formats.cfg");
        return 1;
    }

    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "c:f:hp:")) != -1) {
        switch (c) {
            case 'c':
                cmdConfig = optarg;
                break;
            case 'f':
                cmdFormat = optarg;
                break;
            case 'h':
                PrintHelpAll();
                exit(0);
            case 'p':
                cmdProcess = optarg;
                break;
            case '?':
                if (optopt == 'c' || optopt == 'f') {
                    Logger::Warn("Option -{} requires an argument.", optopt);
                } else {
                    Logger::Warn("Unknown option '-{}'", optopt);
                }
                break;
            default:
                abort();
        }
    }

    if (argc - optind == 1) {
        cmdTool = argv[optind];
    } else if (optind != argc) {
        Logger::Error("Extra arguments");
        return 2;
    }

    g_fmtOutput = cmdFormat;

    if (!ReadSignatureConfig(cmdConfig)) {
        Logger::Error("Failed to read signatures config file \"{}\"", cmdConfig);
        return 3;
    }


    if (!CheckFormat(cmdFormat)) {
        Logger::Error("Unknown format \"{}\"", cmdFormat);
        PrintHelpFormats();
        return 4;
    }

    if (!CheckTool(cmdTool)) {
        Logger::Error("Unknown tool \"{}\"", cmdTool);
        PrintHelpTools();
        return 5;
    }

    if (!g_process.Attach(cmdProcess)) {
        Logger::Error("Failed to find process \"{}\"", cmdProcess);
        Logger::Error("Please ensure the process is running");
        return 6;
    }

    if (!g_process.ParseMaps()) {
        Logger::Error("Failed to parse maps file");
        return 7;
    }

    Logger::Log("Options:");
    PrintOption("Config:", cmdConfig);
    PrintOption("Format:", cmdFormat);
    PrintOption("Process:", cmdProcess);
    PrintOption("Tool:", cmdTool);

    RunTool(cmdTool);

    return 0;
}
