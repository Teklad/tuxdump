#ifndef  __TUXDUMP_FORMATTER_H__
#define  __TUXDUMP_FORMATTER_H__
#include <rapidjson/document.h>

#include <string>
#include <vector>

class Formatter {
    public:
        bool LoadFormat(const char* fmt);
        void Print(const std::string& json, const std::string& label);
    private:
        void Indent();
        void PrintRecursive(rapidjson::Value::ConstMemberIterator object);
    private:
        bool m_bJson = true;
        int m_depth = 0;
        std::string m_fmtTableBegin;
        std::string m_fmtTableEnd;
        std::string m_fmtIndent;
        std::string m_fmtOffset;
        std::string m_fmtHeader;
        std::string m_fmtFooter;
        std::string m_fmtTimestamp;
        std::vector<std::pair<char, char>> m_fmtReplaceChars;
        bool m_fmtRemovePrefix;
};

#endif //__TUXDUMP_FORMATTER_H__
