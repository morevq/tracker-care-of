#pragma once

#include <string>

namespace tracker_api {

    class Config {
    public:
        static Config& getInstance();

        std::string getDbConnInfo() const;
        std::string getRedisUri() const;
        int getApiPort() const;

    private:
        Config() = default;
        
        std::string getEnv(const std::string& key, const std::string& defaultValue = "") const;
    };

} 
