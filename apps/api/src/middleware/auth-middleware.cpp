#include "middleware/auth-middleware.h"
#include <sstream>

namespace tracker_api {

    std::optional<std::string> AuthMiddleware::getUserUuidFromCookie(const crow::request& req) {
        auto cookieHeader = req.get_header_value("Cookie");
        if (cookieHeader.empty()) {
            return std::nullopt;
        }

        std::string sessionUuid = parseCookie(cookieHeader, "session_uuid");
        if (sessionUuid.empty()) {
            return std::nullopt;
        }

        return sessionUuid;
    }

    bool AuthMiddleware::isAuthenticated(const crow::request& req) {
        return getUserUuidFromCookie(req).has_value();
    }

    std::string AuthMiddleware::parseCookie(const std::string& cookieHeader, const std::string& key) {
        std::istringstream stream(cookieHeader);
        std::string token;
    
        while (std::getline(stream, token, ';')) {
            size_t start = token.find_first_not_of(" ");
            if (start != std::string::npos) {
                token = token.substr(start);
            }

            size_t pos = token.find('=');
            if (pos != std::string::npos) {
                std::string cookieKey = token.substr(0, pos);
                std::string cookieValue = token.substr(pos + 1);
            
                if (cookieKey == key) {
                    return cookieValue;
                }
            }
        }
    
        return "";
    }

}