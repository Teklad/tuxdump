#include "netvars.h"

#include <cctype>

std::string CleanPropString(const std::string original)
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
        if (prop.m_pVarName == 0 || isdigit(nvprop.name[0])) {
            continue;
        }

        if (nvprop.name.compare("baseclass") == 0) {
            continue;
        }

        if (prop.m_RecvType == SendPropType::DPT_DataTable && prop.m_pDataTable != 0) {
            auto childTable = ReadMemory<RecvTable>(prop.m_pDataTable);
            table.child_tables.emplace_back(LoadTable(childTable));
            table.child_tables.back().offset = prop.m_Offset;
            table.child_tables.back().prop = nvprop;
        } else {
            table.child_props.emplace_back(nvprop);
        }
    }

    return table;
}

void NetVarManager::Dump(NetVarOutputStyle style)
{
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
            fprintf(m_dumpFile, "constexpr uintptr_t %s = %#lx;\n", cleanName.c_str(), prop.offset);
        }
    }

    for (const NetVar_Table& child : table.child_tables) {
        if (child.offset > 0) {
            size_t sep = child.name.find_first_of('_');
            if (sep != std::string::npos) {
                PrintIndent(indent);
                fprintf(m_dumpFile, "constexpr uintptr_t m%s = %#lx;\n",
                        child.name.c_str() + sep, child.offset);
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
            fprintf(m_dumpFile, "%s [%#lx]\n", prop.name.c_str(), prop.offset);
        }
    }

    for (const NetVar_Table& child : table.child_tables) {
        if (child.offset > 0) {
            size_t sep = child.name.find_first_of('_');
            if (sep != std::string::npos) {
                PrintIndent(indent);
                fprintf(m_dumpFile, "m%s [%#lx]\n",
                        child.name.c_str() + sep, child.offset);
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
