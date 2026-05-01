#pragma once

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

#include <tracker_db/usecases/auth-service.h>

#include "handlers/authenticated-handler.hpp"

namespace tracker_api {

class AuthHandler final : public AuthenticatedHandlerBase {
public:
    static constexpr std::string_view kName = "auth-handler";

    AuthHandler(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

private:
    std::string Register(const userver::server::http::HttpRequest& request) const;
    std::string Login(const userver::server::http::HttpRequest& request) const;
    std::string Logout(const userver::server::http::HttpRequest& request) const;
    std::string UpdateUser(const userver::server::http::HttpRequest& request) const;
    std::string DeleteUser(const userver::server::http::HttpRequest& request) const;

    mutable AuthService auth_service_;
};

}  // namespace tracker_api
