#ifndef __TUXDUMP_NETVAR_H__
#define __TUXDUMP_NETVAR_H__
#include "tprocess/process.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

struct ClientClass {
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
    DPT_NUMSendPropTypes,
    DPT_EmptyDataTable,
    DPT_UnknownDataTable
};

struct RecvProp {
    uintptr_t m_pVarName; // const char*
    SendPropType m_RecvType;
    int m_Flags;
    int m_StringBufferSize;
    bool m_bInsideArray;
    uint8_t pad0[3];
    uintptr_t m_pExtraData;
    uintptr_t m_pArrayProp;
    uintptr_t m_ArrayLengthProxy;
    uintptr_t m_ProxyFn;
    uintptr_t m_DataTableProxyFn;
    uintptr_t m_pDataTable;
    unsigned int m_Offset;
    int m_ElementStride;
    unsigned int m_nElements;
    uint8_t pad1[4];
    uintptr_t m_pParentArrayPropName;
};

struct RecvTable {
    uintptr_t m_pProps;
    unsigned int m_nProps;
    uint8_t pad0[4];
    uintptr_t m_pDecoder;
    uintptr_t m_pNetTableName;
    bool m_bInitialized;
    bool m_bInMainList;
};

class NetvarDumper : public TProcess::Memory {
    public:
        NetvarDumper(TProcess::Process& process);
        ~NetvarDumper() = default;
        void DumpTables(const char* fileName = nullptr);
        void DumpClassIDs(const char* fileName = nullptr);
        void SetHeader(const char* header);
        void SetFooter(const char* footer);
        void SetTableFormat(const char* format);
        void SetPropertyFormat(const char* format);
        void SetCommentFormat(const char* format);
        void SetDefaultDepth(size_t depth);
        void SetShowTablePrefix(bool enabled);
        void SetShowComments(bool enabled);
        void AddSubstitution(char before, char after);
        std::string& SubstituteChars(std::string& str);
    private:
        void DumpTable(std::ostream& dumpFile, RecvTable& recvTable, std::string& tableName, size_t depth = 0);
        void WriteIndent(std::ostream& dumpFile, size_t depth);
        void WriteProperty(std::ostream& dumpFile, std::string& propName, size_t offset, std::string& comment);
        void PrintBufferSize(std::ostream& dumpFile, unsigned int size);
        std::string PropToString(RecvProp& prop, const std::string& propName, size_t offset = 0);
    private:
        std::vector<std::pair<char, char>> m_fmtSubstitutions;
        uintptr_t m_classHead;
        size_t m_defaultDepth;
        bool m_fmtShowComments = false;
        bool m_fmtShowTablePrefix = false;
        const char* m_fmtTable;
        const char* m_fmtProperty;
        const char* m_fmtComment;
        const char* m_fmtHeader = nullptr;
        const char* m_fmtFooter = nullptr;
        const char* m_fmtIndent = "    ";
};

#endif
