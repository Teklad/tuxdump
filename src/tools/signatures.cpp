#include "tools.h"
#include "../globals.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

void Tools::DumpSignatures(Formatter& fmt)
{
    rapidjson::StringBuffer data;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(data);
    writer.StartObject();

    libconfig::Setting& signatures = g_cfg.lookup("signatures");
    for (const libconfig::Setting& entry : signatures) {
        const char *region = entry.lookup("region");
        const char *pattern = entry.lookup("pattern");
        int extra = entry.lookup("extra");
        int relative = entry.lookup("relative");
        libconfig::Setting& offset = entry.lookup("offset");

        TuxProc::Region* currentRegion = g_process.GetRegion(region);
        if (region) {
            uintptr_t addr = g_process.FindPattern(currentRegion, pattern, offset[0]);
            uintptr_t startAddr = 0;
            if (relative) {
                startAddr = currentRegion->GetStartAddress();
                addr = g_process.GetCallAddress(addr);
                for (int i = 1; i < offset.getLength(); ++i) {
                    addr = g_process.Read<uintptr_t>(addr + static_cast<int>(offset[i]));
                }
            } else {
                addr = g_process.Read<int>(addr);
            }
            writer.Key(entry.getName());
            writer.Uint(addr ? addr + extra - startAddr : 0);
        }
    }
    writer.EndObject();
    fmt.Print(data.GetString(), "signatures");
}

