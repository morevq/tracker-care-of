#include "config.h"
#include <cstdlib>
#include <sstream>

namespace tracker_api {

    Config& Config::getInstance() {
        static Config instance;
        return instance;
    }

    std::string Config::getEnv(const std::string& key, const std::string& defaultValue) const {
        const char* val = std::getenv(key.c_str());
        return val ? std::string(val) : defaultValue;
    }

    std::string Config::getDbConnInfo() const {
        std::ostringstream conninfo;
        conninfo << "host=" << getEnv("DB_HOST", "localhost")
                << " port=" << getEnv("DB_PORT", "5432")
                << " dbname=" << getEnv("DB_NAME", "tracker_db")
                << " user=" << getEnv("DB_USER", "postgres")
                << " password=" << getEnv("DB_PASSWORD", "postgres");
        return conninfo.str();
    }

    std::string Config::getRedisUri() const {
        std::string uri = getEnv("REDIS_URI");
        if (!uri.empty()) return uri;

        std::string host = getEnv("REDIS_HOST", "127.0.0.1");
        std::string port = getEnv("REDIS_PORT", "6379");
        return "tcp://" + host + ":" + port;
    }

    int Config::getApiPort() const {
        std::string portStr = getEnv("API_PORT", "8080");
        try {
            return std::stoi(portStr);
        } catch (const std::exception&) {
            return 8080;
        }
    }

}
