#include "netvars.h"

#include <cctype>
#include <cstring>

static std::string CleanPropString(const std::string original)
{
    std::string str = original;
    for (size_t i = 0; i < str.size(); ++i) {
        switch (str[i]) {
            case '.':
                str[i] = '_';
                break;
            case '[':
                str[i] = '_';
                break;
            case ']':
                str.erase(i, 1);
                break;
            default:
                break;
        }
    }
    return str;
}
    
void NetVarManager::PrintIndent(size_t indents)
{
    while (indents-- > 0) {
        fprintf(m_dumpFile, "    ");
    }
}

/**
 * Use some heuristics to help determine the exact type of a variable 
 */
const char* NetVarManager::PropToString(const NetVar_Prop& prop)
{
    switch(prop.type) {
        case SendPropType::DPT_Int:
            break;
        case SendPropType::DPT_Float:
            return "float";
        case SendPropType::DPT_Vector:
            return "Vector";
        case SendPropType::DPT_VectorXY:
            return "VectoryXY";
        case SendPropType::DPT_String:
            return "char";
        case SendPropType::DPT_Array:
            return "array";
        case SendPropType::DPT_Int64:
            return "int64";
        case SendPropType::DPT_DataTable:
            break;
        default:
            break;
    }
    
    if (prop.name[0] == 'm' && prop.name[1] == '_' && !isupper(prop.name[2])) {
        if (prop.name[2] == 'n' && isupper(prop.name[3])) {
            return "unsigned int";
        } else if (prop.name[2] == 'i' && isupper(prop.name[3])) {
            return "int";
        } else if (prop.name[2] == 'b' && isupper(prop.name[3])) {
            return "bool";
        } else if (prop.name.compare(2, 3, "num") == 0 && isupper(prop.name[5])) {
            return "unsigned int";
        } else if (prop.name.compare(2, 2, "fl") == 0 && isupper(prop.name[4])) {
            return "float";
        } else if (prop.name.compare(2, 2, "ch") == 0 && isupper(prop.name[4])) {
            return "char";
        } else if (prop.name.compare(2, 3, "uch") == 0 && isupper(prop.name[5])) {
            return "unsigned char";
        } else if (prop.name.compare(2, 2, "uc") == 0 && isupper(prop.name[4])) {
            return "unsigned char";
        }
    }
    if (prop.type == SendPropType::DPT_DataTable) {
        return "DataTable";
    }
    return "int";
}

std::string NetVarManager::TableToString(const NetVar_Table& table)
{
    char buffer[128];
    snprintf(buffer, 128, "DataTable[%lu] -> ", table.size);
    if (table.typeString == "int") {
        if (table.propSize == 1) {
            if (table.name.compare(0, 3, "m_b") == 0 && isupper(table.name[3])) {
                strcat(buffer, "bool");
            } else {
                strcat(buffer, "char");
            }
        } else if (table.propSize == 2) {
            strcat(buffer, "short");
        } else {
            if (table.name.compare(0, 3, "m_n") == 0 && isupper(table.name[3])) {
                strcat(buffer, "unsigned int");
            } else {
                strcat(buffer, "int");
            }
        }
    } else if (table.typeString == "char") {
        char suffix[10];
        snprintf(suffix, 10, "char[%lu]", table.propSize);
        strcat(buffer, suffix);
    } else if (table.typeString.empty()) {
        strcat(buffer, "Table");
    } else {
        strcat(buffer, table.typeString.c_str());
    }
    return std::string(buffer);
}

NetVarManager::NetVarManager(uintptr_t addr)
{
    ClientClass cc;
    cc.m_pNext = addr;
    do {
        cc = ReadMemory<ClientClass>(cc.m_pNext);
        auto recvTable = ReadMemory<RecvTable>(cc.m_pRecvTable);
        m_data.emplace_back(LoadTable(recvTable));
    } while (cc.m_pNext != 0);
}

NetVarManager::~NetVarManager()
{

}

NetVarManager::NetVar_Table NetVarManager::LoadTable(RecvTable& recvTable)
{
    NetVar_Table table;

    table.name = ReadMemory<std::string>(recvTable.m_pNetTableName, 64);
    table.offset = 0;

    for (int i = 0; i < recvTable.m_nProps; ++i) {
        auto prop = ReadMemory<RecvProp>(recvTable.m_pProps + i * sizeof(RecvProp));
        NetVar_Prop nvprop;
        nvprop.name = ReadMemory<std::string>(prop.m_pVarName, 64);
        nvprop.offset = prop.m_Offset;
        nvprop.type = prop.m_RecvType;
        nvprop.stringSize = prop.m_StringBufferSize;
        if (prop.m_pVarName == 0) {
            continue;
        }
        if (isdigit(nvprop.name[0])) {
            int pNameInt = strtol(nvprop.name.c_str(), nullptr, 10);
            if (pNameInt == 1) {
                table.typeString = PropToString(nvprop);
                table.propSize = prop.m_Offset;
            }
            continue;
        }
        
        if (nvprop.name.compare("baseclass") == 0) {
            continue;
        }

        if (prop.m_RecvType == SendPropType::DPT_DataTable && prop.m_pDataTable != 0) {
            auto childTable = ReadMemory<RecvTable>(prop.m_pDataTable);
            table.child_tables.emplace_back(LoadTable(childTable));
            table.child_tables.back().offset = prop.m_Offset;
            table.child_tables.back().size = childTable.m_nProps;
            table.child_tables.back().prop = nvprop;
        } else {
            table.child_props.emplace_back(nvprop);
        }
    }

    return table;
}

void NetVarManager::Dump(NetVarOutputStyle style, bool comments)
{
    m_useComments = comments;
    std::string fileName = "netvar_output";
    switch(style) {
        case NetVarOutputStyle::CPlusPlus:
            fileName.append(".h");
            break;
        case NetVarOutputStyle::Raw:
            fileName.append(".txt");
            break;
    }

    m_dumpFile = fopen(fileName.c_str(), "w");
    if (!m_dumpFile) {
        fprintf(stderr, "Failed to open '%s' for writing.\n", fileName.c_str());
        return;
    }

    switch(style) {
        case NetVarOutputStyle::CPlusPlus:
            fprintf(m_dumpFile, "#ifndef __GENERATEDNETVARS_H__\n"
                    "#define __GENERATEDNETVARS_H__\n"
                    "#include <cstdint>\n\n"
                    "namespace Netvars {\n");
            break;
        default:
            break;
    }

    for (const NetVar_Table& table : m_data) {
        if (table.child_props.empty() && table.child_tables.empty()) {
            continue;
        }

        switch(style) {
            case NetVarOutputStyle::CPlusPlus:
                PrintIndent();
                if (table.name.compare(0, 3, "DT_") == 0) {
                    fprintf(m_dumpFile, "namespace %s {\n", table.name.c_str() + 3);
                } else {
                    fprintf(m_dumpFile, "namespace %s {\n", table.name.c_str());
                }
                DumpTableCPP(table);
                PrintIndent();
                fprintf(m_dumpFile, "}\n");
                break;
            case NetVarOutputStyle::Raw:
                fprintf(m_dumpFile, "%s\n", table.name.c_str());
                DumpTableRaw(table);
                break;
        }
    }

    switch(style) {
        case NetVarOutputStyle::CPlusPlus:
            fprintf(m_dumpFile, "}\n#endif\n");
            break;
        default:
            break;
    }

    fclose(m_dumpFile);
    fprintf(stderr, "Netvar dump has been written to '%s'.\n", fileName.c_str());
}

void NetVarManager::DumpTableCPP(const NetVar_Table& table, size_t indent)
{
    for (const NetVar_Prop& prop : table.child_props) {
        if (prop.offset > 0) {
            std::string cleanName = CleanPropString(prop.name);
            PrintIndent(indent);
            fprintf(m_dumpFile, "constexpr uintptr_t %s = %#lx;", cleanName.c_str(), prop.offset);
            if (m_useComments) {
                fprintf(m_dumpFile, " // %s", PropToString(prop));
            }
            if (prop.stringSize > 0) {
                fprintf(m_dumpFile, "[%i]", prop.stringSize);
            }
            fprintf(m_dumpFile, "\n");
        }
    }

    for (const NetVar_Table& child : table.child_tables) {
        if (child.offset > 0) {
            size_t sep = child.name.find_first_of('_');
            if (sep != std::string::npos) {
                PrintIndent(indent);
                fprintf(m_dumpFile, "constexpr uintptr_t m%s = %#lx;", child.name.c_str() + sep, child.offset);
                if (m_useComments) {
                    fprintf(m_dumpFile, " // %s", TableToString(child).c_str());
                }
                fprintf(m_dumpFile, "\n");
            }
        }

        if (child.child_props.empty() && child.child_tables.empty()) {
            continue;
        }

        PrintIndent(indent);
        if (child.name.compare(0, 3, "DT_") == 0) {
            fprintf(m_dumpFile, "namespace %s {\n", child.name.c_str() + 3);
        } else {
            fprintf(m_dumpFile, "namespace %s {\n", child.name.c_str());
        }
        DumpTableCPP(child, indent + 1);
        PrintIndent(indent);
        fprintf(m_dumpFile, "}\n");
    }
}

void NetVarManager::DumpTableRaw(const NetVar_Table& table, size_t indent)
{
    for (const NetVar_Prop& prop : table.child_props) {
        if (prop.offset > 0) {
            PrintIndent(indent);
            fprintf(m_dumpFile, "%s [%#lx]", prop.name.c_str(), prop.offset);
            if (m_useComments) {
                fprintf(m_dumpFile, " # %s", PropToString(prop));
            }
            fprintf(m_dumpFile, "\n");
        }
    }

    for (const NetVar_Table& child : table.child_tables) {
        if (child.offset > 0) {
            size_t sep = child.name.find_first_of('_');
            if (sep != std::string::npos) {
                PrintIndent(indent);
                fprintf(m_dumpFile, "m%s [%#lx]",
                        child.name.c_str() + sep, child.offset);
                if (m_useComments) {
                    fprintf(m_dumpFile, " # %s", TableToString(child).c_str());
                }
                fprintf(m_dumpFile, "\n");
            }
        }

        if (child.child_props.empty() && child.child_tables.empty()) {
            continue;
        }

        PrintIndent(indent);
        fprintf(m_dumpFile, "%s\n", child.name.c_str());
        DumpTableRaw(child, indent + 1);
    }
}
