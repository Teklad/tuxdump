#include "tools.h"
#include "../globals.h"
#include "../printer.h"

#include <cstring>

class ClientClass {
    public:
        uintptr_t m_pCreateFn;
        uintptr_t m_pCreateEventF;
        uintptr_t m_pNetworkName;
        uintptr_t m_pRecvTable;
        uintptr_t m_pNext;
        int m_ClassID;
};

enum class SendPropType : int {
    DPT_Int = 0,
    DPT_Float,
    DPT_Vector,
    DPT_VectorXY,
    DPT_String,
    DPT_Array,
    DPT_DataTable,
    DPT_Int64,
    DPT_NUMSendPropTypes
};

class RecvProp {
    public:
        uintptr_t m_pVarName; // const char*
        SendPropType m_RecvType;
        int m_Flags;
        int m_StringBufferSize;
        bool m_bInsideArray;
    private:
        uint8_t pad0[3];
    public:
        uintptr_t m_pExtraData;
        uintptr_t m_pArrayProp;
        uintptr_t m_ArrayLengthProxy;
        uintptr_t m_ProxyFn;
        uintptr_t m_DataTableProxyFn;
        uintptr_t m_pDataTable;
        unsigned int m_Offset;
        int m_ElementStride;
        unsigned int m_nElements;
    private:
        uint8_t pad1[4];
    public:
        uintptr_t m_pParentArrayPropName;
};

class RecvTable {
    public:
        uintptr_t m_pProps;
        unsigned int m_nProps;
    private:
        uint8_t pad0[4];
    public:
        uintptr_t m_pDecoder;
        uintptr_t m_pNetTableName;
        bool m_bInitialized;
};

static uintptr_t GetClassHead()
{
    libconfig::Setting& entry = g_cfg.lookup("signatures.dwGetAllClasses");
    const char* region = entry.lookup("region");
    const char* pattern = entry.lookup("pattern");
    libconfig::Setting& offset = entry.lookup("offset");
    uintptr_t addr = g_process.FindPattern(region, pattern, offset[0]);
    addr = g_process.GetCallAddress(addr);
    for (int i = 1; i < offset.getLength(); ++i) {
        addr = g_process.Read<uintptr_t>(addr + static_cast<int>(offset[i]));
    }
    return addr;
}

static void DumpNetvarTable(RecvTable table, const char* tableName, int depth)
{
    RecvProp props[1024];
    char propName[64];
    if (g_process.ReadMemory(table.m_pProps, props, sizeof(RecvProp) * table.m_nProps) < 1) {
        return;
    }

    // Skip empty classes
    g_process.ReadMemory(props[0].m_pVarName, propName, sizeof(propName));
    if (table.m_nProps == 1) {
        if (!strcmp(propName, "baseclass")) {
            return;
        }
    }

    if (isdigit(propName[0])) {
        return;
    }

    Printer::PrintNetvarTableStart(tableName, depth);
    for (size_t i = 0; i < table.m_nProps; ++i) {
        RecvProp& prop = props[i];
        if (g_process.ReadMemory(prop.m_pVarName, propName, sizeof(propName)) < 1) {
            continue;
        }

        if (!strcmp(propName, "baseclass")) {
            continue;
        }

        if (isdigit(propName[0]) || prop.m_RecvType == SendPropType::DPT_Array) {
            continue;
        }

        if (prop.m_RecvType == SendPropType::DPT_DataTable && prop.m_pDataTable) {
            if (prop.m_Offset > 0) {
                Printer::PrintOffset(propName, prop.m_Offset, depth + 1);
            }
            auto nextTable = g_process.Read<RecvTable>(prop.m_pDataTable);
            char nextTableName[64];
            if (g_process.ReadMemory(nextTable.m_pNetTableName, nextTableName, sizeof(nextTableName)) < 1) {
                continue;
            }
            DumpNetvarTable(nextTable, nextTableName, depth + 1);
        } else {
            Printer::PrintOffset(propName, prop.m_Offset, depth + 1);
        }
    }
    Printer::PrintNetvarTableEnd(tableName, depth);
}

void Tools::DumpNetvars()
{
    Printer::PrintFileHeader("netvars");
    Printer::PrintNetvarsStart();
    char tableName[64];
    ClientClass cc;
    cc.m_pNext = GetClassHead();
    do {
        cc = g_process.Read<ClientClass>(cc.m_pNext);
        if (cc.m_pRecvTable) {
            auto table = g_process.Read<RecvTable>(cc.m_pRecvTable);
            g_process.ReadMemory(cc.m_pNetworkName, tableName, sizeof(tableName));
            DumpNetvarTable(table, tableName, 1);
        }
    } while (cc.m_pNext);
    Printer::PrintNetvarsEnd();
    Printer::PrintFileFooter("netvars");
}

