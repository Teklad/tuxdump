#include "formatter.h"
#include "logger.h"

#include <fmt/time.h>
#include <rapidjson/document.h>
#include <libconfig.h++>

bool Formatter::LoadFormat(const char* fmt)
{
    if (!strcmp(fmt, "json")) {
        return true;
    }

    m_bJson = false;

    libconfig::Config cfg;
    try {
        cfg.readFile("formats.cfg");
    } catch (const libconfig::FileIOException& fioex) {
        Logger::Error("Failed to open file: \"formats.cfg\"");
        return false;
    } catch (const libconfig::ParseException& pex) {
        Logger::Error("Parse exception: {}\nFile: {}:{}\n", 
                pex.getError(), "formats.cfg", pex.getLine());
        return false;
    }

    try {
        libconfig::Setting& formats = cfg.lookup("formats");
        libconfig::Setting& entry = formats.lookup(fmt);

        if (!entry.lookupValue("table_begin", m_fmtTableBegin)) {
            return false;
        }

        if (!entry.lookupValue("table_end", m_fmtTableEnd)) {
            return false;
        }

        if (!entry.lookupValue("offset", m_fmtOffset)) {
            return false;
        }

        entry.lookupValue("strip_table_prefix", m_fmtRemovePrefix);
        entry.lookupValue("header", m_fmtHeader);
        entry.lookupValue("footer", m_fmtFooter);
        entry.lookupValue("timestamp", m_fmtTimestamp);
        entry.lookupValue("indent", m_fmtIndent);
        entry.lookupValue("default_depth", m_depth);
        if (entry.exists("replace_chars")) {
            for (const libconfig::Setting& pair : entry.lookup("replace_chars")) {
                const char* before = pair[0];
                if (pair.getLength() == 2) {
                    const char* after = pair[1];
                    m_fmtReplaceChars.push_back({before[0], after[0]});
                } else {
                    m_fmtReplaceChars.push_back({before[0], 0});
                }
            }
        }
    } catch (const libconfig::SettingNotFoundException& snfex) {
        Logger::Error("{}: {}", snfex.what(), snfex.getPath());
        return false;
    }
    return true;
}

void Formatter::Indent()
{
    for (int depth = 0; depth < m_depth; ++depth) {
        fmt::print(stdout, m_fmtIndent);
    }
}

void Formatter::PrintRecursive(rapidjson::Value::ConstMemberIterator object)
{
    std::string scope = object->name.GetString();


    if (object->value.IsObject()) {
        if (!scope.compare(0, 3, "DT_")) {
            scope.erase(0, 3);
        }
        Indent();
        fmt::print(stdout, m_fmtTableBegin, scope);
        m_depth++;
        for (auto it = object->value.MemberBegin(); it != object->value.MemberEnd(); ++it) {
            PrintRecursive(it);
        }
        m_depth--;
        Indent();
        fmt::print(stdout, m_fmtTableEnd, scope);
    } else if (object->value.IsInt()) {
        for (const std::pair<char, char>& replacements : m_fmtReplaceChars) {
            size_t pos = scope.find(replacements.first);
            while (pos != std::string::npos) {
                if (!replacements.second) {
                    scope.erase(pos, 1);
                    pos = scope.find(replacements.first, pos);
                } else {
                    scope[pos] = replacements.second;
                    pos = scope.find(replacements.first, pos + 1);
                }

            }
        }
        Indent();
        fmt::print(stdout, m_fmtOffset, scope, object->value.GetUint());
    }
}

void Formatter::Print(const std::string& json, const std::string& label)
{
    if (m_bJson) {
        puts(json.c_str());
        return;
    }

    try {
        fmt::print(stdout, m_fmtHeader, label);
    } catch(const fmt::v5::format_error& fex) {
        Logger::Warn("Header didn't supply {{0}}.  Writing without input.");
        fmt::print(stdout, m_fmtHeader);
    }

    Indent();
    try {
        std::time_t t = std::time(nullptr);
        fmt::print(stdout, m_fmtTimestamp, *std::localtime(&t), "checkers");
    } catch (const fmt::format_error& fex) {
        Logger::Warn("Timestamp format missing or invalid. Skipping.");
    }

    rapidjson::Document doc;
    doc.Parse(json.c_str());
    for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
        PrintRecursive(it);
    }

    try {
        fmt::print(stdout, m_fmtFooter, label);
    } catch(const fmt::v5::format_error& fex) {
        Logger::Warn("Footer didn't supply {{0}}.  Writing without input.");
        fmt::print(stdout, m_fmtFooter);
    }
}
