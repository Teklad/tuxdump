#include "netvars.h"

#include <cctype>
#include <cstring>
#include <iostream>

static inline std::string& StringReplace(std::string& str,
        const std::string& before,
        const std::string& after)
{
    size_t pos = str.find(before);
    while (pos != std::string::npos) {
        str.replace(pos, before.size(), after);
        pos = str.find(before, pos + after.size());
    }
    return str;
}

/**
 * @brief Given a string, returns the portions before and after the delimiter
 *        where before is stored in the original string and after is returned
 *        as a new string
 *
 * @param str The original string
 * @param delimiter
 *
 * @return after
 */
static inline std::string StringSplit(std::string& str,
        const std::string& delimiter)
{
    size_t pos = str.find(delimiter);
    if (pos != std::string::npos) {
        std::string after = str.substr(pos + delimiter.size());
        str.erase(pos);
        return after;
    }
    return std::string();
}

static inline std::string OffsetToHex(unsigned int offset)
{
    char buf[16];
    snprintf(buf, 16, "%#x", offset);
    return buf;
}

NetvarDumper::NetvarDumper(TProcess::Process& process)
{
    TProcess::Region& client = process.GetRegion("client_panorama_client.so");
    uintptr_t classHead = client.Find("488b..........8b....48....48....75..e9........66", 2);
    classHead = client.GetCallAddress(classHead);
    classHead = process.ReadMemory<uintptr_t>(classHead);
    classHead = process.ReadMemory<uintptr_t>(classHead);
    m_classHead = classHead;
}

void NetvarDumper::SetTableFormat(const char* format)
{
    m_fmtTable = format;
}

void NetvarDumper::SetPropertyFormat(const char* format)
{
    m_fmtProperty = format;
}

void NetvarDumper::SetCommentFormat(const char* format)
{
    m_fmtComment = format;
}

void NetvarDumper::SetHeader(const char* header)
{
    m_fmtHeader = header;
}

void NetvarDumper::SetFooter(const char* footer)
{
    m_fmtFooter = footer;
}

void NetvarDumper::SetDefaultDepth(size_t depth)
{
    m_defaultDepth = depth;
}

void NetvarDumper::SetShowComments(bool enabled)
{
    m_fmtShowComments = enabled;
}

void NetvarDumper::SetShowTablePrefix(bool enabled)
{
    m_fmtShowTablePrefix = enabled;
}

void NetvarDumper::AddSubstitution(char before, char after)
{
    m_fmtSubstitutions.push_back({before, after});
}

std::string& NetvarDumper::SubstituteChars(std::string& str)
{
    for (size_t i = 0; i < str.size(); ++i) {
        for (auto& c : m_fmtSubstitutions) {
            if (str[i] == c.first) {
                if (c.second == 0) {
                    str.erase(i, 1);
                } else {
                    str[i] = c.second;
                }
            }
        }
    }
    return str;
}

std::string NetvarDumper::PropToString(RecvProp& prop, const std::string& propName, size_t offset)
{
    switch(prop.m_RecvType) {
        case SendPropType::DPT_Int:
            break;
        case SendPropType::DPT_Float:
            return "float";
        case SendPropType::DPT_String:
            return "char";
        case SendPropType::DPT_Vector:
            return "Vector";
        case SendPropType::DPT_VectorXY:
            return "VectorXY";
        default:
            return "unk";
    }

    if (propName.compare(0, 3, "m_b") == 0 && isupper(propName[3])) {
        return "bool";
    } else if (propName.compare(0, 3, "m_i") == 0 && isupper(propName[3])) {
        return "int";
    } else if (propName.compare(0, 3, "m_n") == 0 && isupper(propName[3])) {
        return "unsigned int";
    } else if (propName.compare(0, 4, "m_ch") == 0 && isupper(propName[4])) {
        return "char";
    } else if (propName.compare(0, 4, "m_uc") == 0 && isupper(propName[4])) {
        return "unsigned char";
    } else if (propName.compare(0, 5, "m_uch") == 0 && isupper(propName[5])) {
        return "unsigned char";
    } else if (propName.compare(0, 4, "m_ub") == 0 && isupper(propName[4])) {
        return "unsigned char";
    } else if (propName.compare(0, 4, "m_fl") == 0 && isupper(propName[4])) {
        return "float";
    }

    if (offset > 0) {
        switch(offset) {
            case 1:
                return "bool";
            case 2:
                return "short";
            case 8:
                return "uintptr_t";
            default:
                return "int";
        }
    }
    return "int";
}

void NetvarDumper::DumpTables(const char* fileName)
{
    std::ofstream dumpFile(fileName);
    if (dumpFile.is_open()) {
        if (m_fmtHeader) {
            dumpFile << m_fmtHeader;
        }
        ClientClass cc;
        cc.m_pNext = m_classHead;
        do {
            cc = ReadMemory<ClientClass>(cc.m_pNext);
            if (cc.m_pRecvTable != 0) {
                auto table = ReadMemory<RecvTable>(cc.m_pRecvTable);
                auto tableName = ReadMemory<std::string>(cc.m_pNetworkName, 64);
                DumpTable(dumpFile, table, tableName, m_defaultDepth);
            }
        } while (cc.m_pNext != 0);
        if (m_fmtFooter) {
            dumpFile << m_fmtFooter;
        }
        dumpFile.close();
    }
}

void NetvarDumper::WriteIndent(std::ostream& dumpFile, size_t depth)
{
    for (size_t i = 0; i < depth; ++i) {
        dumpFile << m_fmtIndent;
    }
}

void NetvarDumper::WriteProperty(std::ostream& dumpFile,
        std::string& propName,
        size_t offset,
        std::string& comment)
{
    std::string propertyFormat = m_fmtProperty;
    std::string commentFormat = m_fmtComment;
    SubstituteChars(propName);
    StringReplace(propertyFormat, "{{NAME}}", propName);
    StringReplace(propertyFormat, "{{VALUE}}", OffsetToHex(offset));
    if (m_fmtShowComments) {
        StringReplace(commentFormat, "{{COMMENT}}", comment);
        StringReplace(propertyFormat, "{{COMMENT}}", commentFormat);
    } else {
        StringReplace(propertyFormat, "{{COMMENT}}", "");
    }
    dumpFile << propertyFormat;
}

// Maybe come up with a way to simplify this a bit.
void NetvarDumper::DumpTable(std::ostream& dumpFile, RecvTable& table, std::string& tableName, size_t depth)
{
    RecvProp props[1024];
    if (!ReadMemoryToBuffer(table.m_pProps, props, sizeof(RecvProp) * table.m_nProps)) {
        return;
    }

    // Skip empty classes
    auto propName = ReadMemory<std::string>(props[0].m_pVarName, 64);
    if (table.m_nProps == 1) {
        if (propName == "baseclass") {
            return;
        }
    }

    std::string tableBegin = m_fmtTable;
    if (m_fmtShowComments) {
        size_t totalProps = table.m_nProps - (propName == "baseclass");
        std::string tableComment = m_fmtComment;
        StringReplace(tableComment, "{{COMMENT}}", std::string("DataTable[") + std::to_string(totalProps) + "]");
        StringReplace(tableBegin, "{{COMMENT}}", tableComment);
    } else {
        StringReplace(tableBegin, "{{COMMENT}}", "");
    }
    if (tableName.compare(0, 3, "DT_") == 0 && m_fmtShowTablePrefix == false) {
        tableName.erase(0, 3);
    }
    StringReplace(tableBegin, "{{NAME}}", tableName);
    std::string tableEnd = StringSplit(tableBegin, "{{DATA}}");
    WriteIndent(dumpFile, depth);
    dumpFile << tableBegin;

    for (size_t i = 0; i < table.m_nProps; ++i) {
        RecvProp& prop = props[i];
        propName = ReadMemory<std::string>(prop.m_pVarName, 64);

        if (propName == "baseclass") {
            continue;
        }

        if (isdigit(propName[0]) && prop.m_RecvType != SendPropType::DPT_DataTable) {
            continue;
        }

        if (strtol(propName.c_str(), nullptr, 10) > 0 && prop.m_RecvType == SendPropType::DPT_DataTable) {
            continue;
        }

        if (prop.m_RecvType == SendPropType::DPT_Array) {
            continue;
        }

        bool skipTable = false;

        std::string propInfo;
        if (prop.m_RecvType == SendPropType::DPT_DataTable && prop.m_pDataTable) {
            auto nextTable = ReadMemory<RecvTable>(prop.m_pDataTable);
            RecvProp nextProp;
            if (nextTable.m_nProps > 1) {
                nextProp = ReadMemory<RecvProp>(nextTable.m_pProps + sizeof(RecvProp));
            } else {
                nextProp = ReadMemory<RecvProp>(nextTable.m_pProps);
            }
            propInfo = std::string("DataTable[") + std::to_string(nextTable.m_nProps) + "]";
            auto nextTableName = ReadMemory<std::string>(nextTable.m_pNetTableName, 64);
            auto nextPropName = ReadMemory<std::string>(nextProp.m_pVarName, 64);
            if (nextPropName == "000" || nextPropName == "001") {
                if (nextProp.m_RecvType != SendPropType::DPT_DataTable) {
                    skipTable = true;
                }
                propInfo.append(" - ");
                propInfo.append(PropToString(nextProp, propName, nextProp.m_Offset));
                if (nextProp.m_RecvType == SendPropType::DPT_String && nextProp.m_StringBufferSize > 0) {
                    propInfo.append(std::string() + '[' + std::to_string(nextProp.m_StringBufferSize) + ']');
                }
            }

            if (prop.m_Offset > 0) {
                WriteIndent(dumpFile, depth + 1);
                WriteProperty(dumpFile, propName, prop.m_Offset, propInfo);
            }

            if (!skipTable) {
                DumpTable(dumpFile, nextTable, nextTableName, depth + 1);
            }

        } else {
            propInfo = PropToString(prop, propName);
            if (prop.m_RecvType == SendPropType::DPT_String && prop.m_StringBufferSize > 0) {
                propInfo.append(std::string() + '[' + std::to_string(prop.m_StringBufferSize) + ']');
            }
            WriteIndent(dumpFile, depth + 1);
            WriteProperty(dumpFile, propName, prop.m_Offset, propInfo);
        }
    }
    if (tableEnd.size()) {
        WriteIndent(dumpFile, depth);
        dumpFile << tableEnd;
    }
}
