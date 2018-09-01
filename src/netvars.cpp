#include "netvars.h"

#include <cctype>
#include <cstring>

static void CleanPropString(char* str)
{
    char *chp = str;
    while (*chp != 0) {
        switch (*chp) {
            case '.':
                *chp = '_';
                chp++;
                break;
            case '[':
                *chp = '_';
                chp++;
                break;
            case ']':
                memmove(chp, chp + 1, str + 64 - chp - 1);
                break;
            default:
                chp++;
                break;
        }
    }
}
    
void NetVarManager::PrintIndent(size_t indents)
{
    for (size_t i = 0; i < indents; ++i) {
        fprintf(m_dumpFile, "    ");
    }
}

NetVarManager::NetVarManager(int pid, uintptr_t addr)
{
    this->SetPID(pid);
    auto cc = this->Read<ClientClass>(addr);
    while (true) {
        auto recvTable = this->Read<RecvTable>(cc.m_pRecvTable);
        m_data.emplace_back(this->LoadTable(recvTable));
        if (cc.m_pNext == 0) {
            break;
        }
        cc = this->Read<ClientClass>(cc.m_pNext);
    }
}

NetVarManager::~NetVarManager()
{

}

NetVarManager::NetVar_Table NetVarManager::LoadTable(RecvTable& recvTable)
{
    NetVar_Table table;

    table.offset = 0;
    this->ReadArray(recvTable.m_pNetTableName, table.name, 64);

    for (int i = 0; i < recvTable.m_nProps; ++i) {
        char propName[64];
        auto prop = this->Read<RecvProp>(recvTable.m_pProps + i * sizeof(RecvProp));
        this->ReadArray(prop.m_pVarName, propName, 64);
        if (!prop.m_pVarName || isdigit(propName[0])) {
            continue;
        }

        if (strcmp(propName, "baseclass") == 0) {
            continue;
        }

        if (prop.m_RecvType == SendPropType::DPT_DataTable && prop.m_pDataTable != 0) {
            auto childTable = this->Read<RecvTable>(prop.m_pDataTable);
            table.child_tables.emplace_back(this->LoadTable(childTable));
            table.child_tables.back().offset = prop.m_Offset;
            memcpy(table.child_tables.back().prop.name, propName, 64);
            table.child_tables.back().prop.offset = prop.m_Offset;
        } else {
            NetVar_Prop nvprop;
            nvprop.offset = prop.m_Offset;
            memcpy(nvprop.name, propName, 64);
            table.child_props.emplace_back(nvprop);
        }
    }

    return table;
}

void NetVarManager::Dump(NetVarOutputStyle style)
{
    constexpr char nvfile[] = "netvar_output";
    char fileName[64];
    switch(style) {
        case NetVarOutputStyle::CPlusPlus:
            snprintf(fileName, 64, "%s.h", nvfile);
            this->m_style = NetVarOutputStyle::CPlusPlus;
            break;
        case NetVarOutputStyle::Raw:
            snprintf(fileName, 64, "%s.txt", nvfile);
            this->m_style = NetVarOutputStyle::Raw;
            break;
    }
    m_dumpFile = fopen(fileName, "w");
    if (!m_dumpFile) {
        fprintf(stderr, "Failed to open '%s' for writing.\n", fileName);
        return;
    }

    if (m_style == NetVarOutputStyle::CPlusPlus) {
        fprintf(m_dumpFile, "#ifndef __GENERATEDNETVARS_H__\n");
        fprintf(m_dumpFile, "#define __GENERATEDNETVARS_H__\n");
        fprintf(m_dumpFile, "#include <cstdint>\n\n");
        fprintf(m_dumpFile, "namespace Netvars {\n");
    }
    for (auto& table : m_data) {
        if (table.child_props.empty() && table.child_tables.empty()) {
            continue;
        }
        switch(m_style) {
            case NetVarOutputStyle::CPlusPlus:
                fprintf(m_dumpFile, "namespace %s {\n", table.name);
                DumpTableCPP(table, 1);
                fprintf(m_dumpFile, "}\n");
                break;
            case NetVarOutputStyle::Raw:
                fprintf(m_dumpFile, "%s\n", table.name);
                DumpTableRaw(table, 1);
                break;
        }
    }
    if (m_style == NetVarOutputStyle::CPlusPlus) {
        fprintf(m_dumpFile, "}\n#endif\n");
    }
    fclose(m_dumpFile);
    fprintf(stderr, "Netvar dump has been written to '%s'.\n", fileName);
}

void NetVarManager::DumpTableCPP(const NetVar_Table& table, size_t indent)
{
    for (const NetVar_Prop& prop : table.child_props) {
        if (prop.offset > 0) {
            char sanitizedName[64];
            strcpy(sanitizedName, prop.name);
            CleanPropString(sanitizedName);
            PrintIndent(indent);
            fprintf(m_dumpFile, "constexpr uintptr_t %s = %#lx;\n", sanitizedName, prop.offset);
        }
    }

    for (const NetVar_Table& child : table.child_tables) {
        if (child.offset > 0) {
            const char* sep = strstr(child.name, "_");
            if (sep != NULL) {
                PrintIndent(indent);
                fprintf(m_dumpFile, "constexpr uintptr_t m%s = %#lx;\n", child.name + (sep - child.name), child.offset);
            }
        }
        if (child.child_props.empty() && child.child_tables.empty()) {
            continue;
        }
        PrintIndent(indent);
        fprintf(m_dumpFile, "namespace %s {\n", child.name);
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
            fprintf(m_dumpFile, "%s [%#lx]\n", prop.name, prop.offset);
        }
    }

    for (const NetVar_Table& child : table.child_tables) {
        if (child.offset > 0) {
            const char* sep = strstr(child.name, "_");
            if (sep != NULL) {
                PrintIndent(indent);
                fprintf(m_dumpFile, "m%s [%#lx]\n", child.name + (sep - child.name), child.offset);
            }
        }
        if (child.child_props.empty() && child.child_tables.empty()) {
            continue;
        }
        PrintIndent(indent);
        fprintf(m_dumpFile, "%s\n", child.name);
        DumpTableRaw(child, indent + 1);
    }
}
