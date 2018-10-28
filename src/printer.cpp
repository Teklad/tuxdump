#include "printer.h"
#include "globals.h"

#include <fmt/format.h>
#include <fmt/time.h>
#include <chrono>

static void RemoveChars(char* str, char before)
{
    char* pw = str;
    while (*str) {
        *pw = *str++;
        pw += (*pw != before);
    }
    *pw = 0;
}

static void ReplaceChars(char* str, char before, char after)
{
    for(char* pr = str; *pr != 0; ++pr) {
        if (*pr == before) {
            *pr = after;
        }
    }
}

void Printer::PrintDepth(int depth)
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    try {
        const char* delimiter = style.lookup("tab_character");
        while (depth > 0) {--depth; fmt::print(stdout, delimiter);}
    } catch (libconfig::SettingNotFoundException&) {
    }
}

void Printer::PrintOffset(const char* name, size_t offset, int depth)
{
    PrintDepth(depth);
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    const char* format = style.lookup("variable_offset");
    try {
        libconfig::Setting& replacements = style.lookup("netvars.replace_chars");
        char varName[256];
        strcpy(varName, name);
        for (libconfig::Setting& value : replacements) {
            const char* before = value[0];
            if (value.getLength() == 1) {
                RemoveChars(varName, before[0]);
            } else {
                const char* after = value[1];
                ReplaceChars(varName, before[0], after[0]);
            }
        }
        fmt::print(stdout, format, varName, offset);
    } catch (libconfig::SettingNotFoundException&) {
        fmt::print(stdout, format, name, offset);
    }
}

void Printer::PrintFileHeader(const char* name)
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    try {
        const char* format = style.lookup("all.start");
        fmt::print(stdout, format, name);
    } catch (libconfig::SettingNotFoundException&){}
}

void Printer::PrintFileFooter(const char* name)
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    try {
        const char* format = style.lookup("all.end");
        fmt::print(stdout, format, name);
    } catch (libconfig::SettingNotFoundException&){}
}

void Printer::PrintSignaturesStart()
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    try {
        const char* format = style.lookup("signatures.start");
        fmt::print(stdout, format);
    } catch (libconfig::SettingNotFoundException&){}
}

void Printer::PrintSignaturesEnd()
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    try {
        const char* format = style.lookup("signatures.end");
        fmt::print(stdout, format);
    } catch (libconfig::SettingNotFoundException&){}
}

void Printer::PrintNetvarsStart()
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    try {
        const char* format = style.lookup("netvars.start");
        fmt::print(stdout, format);
    } catch (libconfig::SettingNotFoundException&){}
}


void Printer::PrintNetvarsEnd()
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    try {
        const char* format = style.lookup("netvars.end");
        fmt::print(stdout, format);
    } catch (libconfig::SettingNotFoundException&){}
}

void Printer::PrintNetvarTableStart(const char* name, int depth)
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    if (style.exists("netvars.table_start")) {
        PrintDepth(depth);
        bool stripPrefix = false;
        const char* format = style.lookup("netvars.table_start");

        try {
            stripPrefix = style.lookup("netvars.strip_table_prefix");
        } catch (libconfig::SettingNotFoundException&){}

        if (stripPrefix) {
            char tableName[256];
            strcpy(tableName, name);
            if (!strncmp(tableName, "DT_", 3)) {
                memmove(tableName, tableName + 3, sizeof(tableName) - 3);
            }
            fmt::print(stdout, format, tableName);
        } else {
            fmt::print(stdout, format, name);
        }
    }
}

void Printer::PrintNetvarTableEnd(const char* name, int depth)
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    try {
        PrintDepth(depth);
        const char* format = style.lookup("netvars.table_end");
        fmt::print(stdout, format, name);
    } catch (libconfig::SettingNotFoundException&){}
}

void Printer::PrintTimestamp()
{
    libconfig::Setting& style = g_cfgFormat.lookup("formats").lookup(g_fmtOutput);
    try {
        const char* format = style.lookup("timestamp");
        std::time_t t = std::time(nullptr);
        fmt::print(stdout, format, *std::localtime(&t));
    } catch (libconfig::SettingNotFoundException&) {}
}

