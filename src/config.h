#pragma once

#include <libconfig.h++>
#include <vector>

struct SignatureDefinition_t {
    std::string name;
    std::string module;
    std::string pattern;
    int offset = 0;
    int extra = 0;
    int relative = 0;
};

typedef std::vector<SignatureDefinition_t> signatures_t;

class TConfig {
    public:
        TConfig() = default;
        ~TConfig() = default;
        TConfig(const TConfig&) = delete;
        TConfig& operator=(const TConfig&) = delete;
        bool ReadFile(const char* file);
        bool ReadFile(const std::string& file);
        std::string GetName();
        std::string GetVersion();
        signatures_t GetSignatures();
    private:
        libconfig::Config m_cfg;
};
