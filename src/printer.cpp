#include "printer.h"
#include "globals.h"

#include <fmt/format.h>
#include <fmt/time.h>
#include <chrono>

void Printer::PrintDepth(int depth)
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    if (style.exists("depth_delimiter")) {
        const char* delimiter = style.lookup("depth_delimiter");
        while (depth > 0) {--depth; fmt::print(stdout, delimiter);}
    }
}

void Printer::PrintOffset(const char* name, size_t offset, int depth)
{
    PrintDepth(depth);
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    const char* format = style.lookup("variable_offset");
    fmt::print(stdout, format, name, offset);
}

void Printer::PrintFileHeader(const char* name)
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    if (style.exists("file_start")) {
        const char* format = style.lookup("file_start");
        fmt::print(stdout, format, name);
    }
}

void Printer::PrintFileFooter(const char* name)
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    if (style.exists("file_end")) {
        const char* format = style.lookup("file_end");
        fmt::print(stdout, format, name);
    }
}

void Printer::PrintSignaturesStart()
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    if (style.exists("signatures_start")) {
        const char* format = style.lookup("signatures_start");
        fmt::print(stdout, format);
    }
}

void Printer::PrintSignaturesEnd()
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    if (style.exists("signatures_end")) {
        const char* format = style.lookup("signatures_end");
        fmt::print(stdout, format);
    }
}

void Printer::PrintNetvarsStart()
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    if (style.exists("netvars_start")) {
        const char* format = style.lookup("netvars_start");
        fmt::print(stdout, format);
    }
}


void Printer::PrintNetvarsEnd()
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    if (style.exists("netvars_end")) {
        const char* format = style.lookup("netvars_end");
        fmt::print(stdout, format);
    }
}

void Printer::PrintNetvarTableStart(const char* name, int depth)
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    if (style.exists("netvar_table_start")) {
        PrintDepth(depth);
        const char* format = style.lookup("netvar_table_start");
        fmt::print(stdout, format, name);
    }
}

void Printer::PrintNetvarTableEnd(const char* name, int depth)
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    if (style.exists("netvar_table_end")) {
        PrintDepth(depth);
        const char* format = style.lookup("netvar_table_end");
        fmt::print(stdout, format, name);
    }
}

void Printer::PrintTimestamp()
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    if (style.exists("timestamp")) {
        const char* format = style.lookup("timestamp");
        std::time_t t = std::time(nullptr);
        fmt::print(stdout, format, *std::localtime(&t));
    }
}

