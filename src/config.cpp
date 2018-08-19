#include "config.h"

bool TConfig::ReadFile(const char* file)
{
    try {
        m_cfg.readFile(file);
        return true;
    } catch (const libconfig::FileIOException& fioex) {
        fprintf(stderr, "%s: I/O error while reading file '%s'\n", fioex.what(), file);
    } catch (const libconfig::ParseException& pex) {
        fprintf(stderr, "%s: Parse error at %s:%i - %s\n", pex.what(),
                pex.getFile(),
                pex.getLine(), 
                pex.getError());
    }

    return false;
}

bool TConfig::ReadFile(const std::string& file)
{
    return ReadFile(file.c_str());
}

std::string TConfig::GetName()
{
    const libconfig::Setting& root = m_cfg.getRoot();
    std::string result = "Unknown";
    if (root.exists("name")) {
        root.lookupValue("name", result);
    }
    return result;
}

std::string TConfig::GetVersion()
{
    const libconfig::Setting& root = m_cfg.getRoot();
    std::string result = "Unknown";
    if (root.exists("version")) {
        root.lookupValue("version", result);
    }
    return result;
}

signatures_t TConfig::GetSignatures()
{
    const libconfig::Setting& root = m_cfg.getRoot();
    signatures_t result;
    try {
        const libconfig::Setting& sigRoot = root["signatures"];
        for (auto&& def : sigRoot) {
            if (!def.exists("module")) {
                fprintf(stderr, "WARN: '%s' missing module entry. Skipping.", def.getName());
                continue;
            }
            if (!def.exists("pattern")) {
                fprintf(stderr, "WARN: '%s' missing pattern entry. Skipping.", def.getName());
                continue;
            }

            SignatureDefinition_t sig;
            sig.name = def.getName();
            def.lookupValue("module", sig.module);
            def.lookupValue("pattern", sig.pattern);

            if (def.exists("offset")) {
                def.lookupValue("offset", sig.offset);
            }
            if (def.exists("extra")) {
                def.lookupValue("extra", sig.extra);
            }
            if (def.exists("relative")) {
                def.lookupValue("relative", sig.relative);
            }
            result.push_back(std::move(sig));
        }
    } catch (const libconfig::SettingNotFoundException& e) {
        fprintf(stderr, "%s: Missing 'signatures' group in config file.\n", e.what());
    }
    return result;
}

