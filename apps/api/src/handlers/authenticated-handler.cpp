#include "handlers/authenticated-handler.hpp"

namespace tracker_api {

AuthenticatedHandlerBase::AuthenticatedHandlerBase(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context,
    tracker_session::SessionStore& session_store)
    : HttpHandlerBase(config, context),
      session_store_(session_store) {}

std::optional<std::string> AuthenticatedHandlerBase::GetSessionId(
    const userver::server::http::HttpRequest& request) const {
    const auto& sid = request.GetCookie(kCookieName);
    if (sid.empty()) return std::nullopt;
    return sid;
}

std::optional<std::string> AuthenticatedHandlerBase::GetCurrentUserUuid(
    const userver::server::http::HttpRequest& request) const {
    auto sid = GetSessionId(request);
    if (!sid) return std::nullopt;
    return session_store_.resolveUserUuid(*sid);
}

}  // namespace tracker_api
