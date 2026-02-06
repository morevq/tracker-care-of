#pragma once
#include <crow.h>
#include <optional>
#include <memory>
#include <string>

#include <tracker_session/session-store.h>

namespace tracker_api {

    class AuthMiddleware {
    public:
        static void init(std::shared_ptr<tracker_session::SessionStore> store);

        static std::optional<std::string> getUserUuidFromCookie(const crow::request& req);

        static bool isAuthenticated(const crow::request& req);

        static std::optional<std::string> getSessionIdFromCookie(const crow::request& req);

    private:
        static std::shared_ptr<tracker_session::SessionStore> store_;
    };

}