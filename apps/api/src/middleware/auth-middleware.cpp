#include "middleware/auth-middleware.h"
#include <tracker_session/cookie.h>

namespace tracker_api {

    std::shared_ptr<tracker_session::SessionStore> AuthMiddleware::store_ = nullptr;

    void AuthMiddleware::init(std::shared_ptr<tracker_session::SessionStore> store) {
        store_ = std::move(store);
    }

    std::optional<std::string> AuthMiddleware::getSessionIdFromCookie(const crow::request& req) {
        auto cookieHeader = req.get_header_value("Cookie");
        if (cookieHeader.empty()) return std::nullopt;

        auto sid = tracker_session::cookie::getCookie(cookieHeader, "__Host-session");
        if (!sid || sid->empty()) return std::nullopt;

        return sid;
    }

    std::optional<std::string> AuthMiddleware::getUserUuidFromCookie(const crow::request& req) {
        if (!store_) return std::nullopt;

        auto sid = getSessionIdFromCookie(req);
        if (!sid) return std::nullopt;

        return store_->resolveUserUuid(*sid);
    }

    bool AuthMiddleware::isAuthenticated(const crow::request& req) {
        return getUserUuidFromCookie(req).has_value();
    }

}