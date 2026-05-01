#pragma once

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>

#include <tracker_session/session-store.h>

#include <optional>
#include <string>

namespace tracker_api {

class AuthenticatedHandlerBase
    : public userver::server::handlers::HttpHandlerBase {
public:
    AuthenticatedHandlerBase(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context,
        tracker_session::SessionStore& session_store);

    static constexpr const char* kCookieName = "session_uuid";
    static constexpr int kSessionTtlSeconds = 86400;
    static constexpr bool kCookieHttpOnly = true;
    static constexpr bool kCookieSecure = false;
    static constexpr const char* kSameSite = "Strict";

protected:
    std::optional<std::string> GetSessionId(
        const userver::server::http::HttpRequest& request) const;

    std::optional<std::string> GetCurrentUserUuid(
        const userver::server::http::HttpRequest& request) const;

    tracker_session::SessionStore& session_store() const noexcept {
        return session_store_;
    }

private:
    tracker_session::SessionStore& session_store_;
};

}  // namespace tracker_api
