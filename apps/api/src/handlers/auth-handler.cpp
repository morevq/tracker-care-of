#include "handlers/auth-handler.hpp"

#include <nlohmann/json.hpp>

#include <userver/storages/postgres/component.hpp>

#include <tracker_crypto/password-hasher.h>
#include <tracker_db/repositories/user-repository.h>
#include <tracker_session/cookie.h>

#include "../dto/auth-dto.h"
#include "components/session-store-component.hpp"
#include "handlers/http-helpers.hpp"

namespace tracker_api {

namespace {

using json = nlohmann::json;

void SetSessionCookie(
    const userver::server::http::HttpRequest& request,
    const std::string& sid) {
    request.GetHttpResponse().SetHeader(
        std::string_view{"Set-Cookie"},
        tracker_session::cookie::buildSetCookie(
            AuthenticatedHandlerBase::kCookieName, sid,
            AuthenticatedHandlerBase::kSessionTtlSeconds,
            AuthenticatedHandlerBase::kCookieHttpOnly,
            AuthenticatedHandlerBase::kCookieSecure,
            AuthenticatedHandlerBase::kSameSite, "/"));
}

void ClearSessionCookie(const userver::server::http::HttpRequest& request) {
    request.GetHttpResponse().SetHeader(
        std::string_view{"Set-Cookie"},
        tracker_session::cookie::buildDeleteCookie(
            AuthenticatedHandlerBase::kCookieName,
            AuthenticatedHandlerBase::kCookieHttpOnly,
            AuthenticatedHandlerBase::kCookieSecure,
            AuthenticatedHandlerBase::kSameSite, "/"));
}

void SetJsonContentType(const userver::server::http::HttpRequest& request) {
    request.GetHttpResponse().SetHeader(
        std::string_view{"Content-Type"}, std::string{"application/json"});
}

}  // namespace

AuthHandler::AuthHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : AuthenticatedHandlerBase(
          config, context,
          context.FindComponent<SessionStoreComponent>().GetStore()),
      auth_service_(context
                        .FindComponent<userver::components::Postgres>(
                            "postgres-db")
                        .GetCluster()) {}

std::string AuthHandler::GetRequestBodyForLogging(
    const userver::server::http::HttpRequest&,
    userver::server::request::RequestContext&,
    const std::string& request_body) const {
    try {
        auto data = json::parse(request_body);
        if (data.is_object()) {
            for (const auto& key :
                 {"password", "current_password", "new_password"}) {
                if (data.contains(key)) {
                    data[key] = "***";
                }
            }
        }
        return data.dump();
    } catch (const std::exception&) {
        return "[redacted]";
    }
}

std::string AuthHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    const auto& action = request.GetPathArg("action");
    const auto method = request.GetMethod();
    using Method = userver::server::http::HttpMethod;

    if (action == "register" && method == Method::kPost) {
        return Register(request);
    }
    if (action == "login" && method == Method::kPost) {
        return Login(request);
    }
    if (action == "logout" && method == Method::kPost) {
        return Logout(request);
    }
    if (action == "me" && method == Method::kGet) {
        return Me(request);
    }
    if (action == "change-password" && method == Method::kPost) {
        return ChangePassword(request);
    }
    if (action == "user" && method == Method::kPatch) {
        return UpdateUser(request);
    }
    if (action == "user" && method == Method::kDelete) {
        return DeleteUser(request);
    }

    return http::SetStatus(
        request, userver::server::http::HttpStatus::kMethodNotAllowed,
        "Method not allowed");
}

std::string AuthHandler::Register(
    const userver::server::http::HttpRequest& request) const {
    try {
        auto data = json::parse(request.RequestBody());
        auto req = data.get<RegisterRequest>();

        auto userUuid = auth_service_.registerUser(req.email, req.password);
        if (!userUuid) {
            return http::BadRequest(
                request, "User already exists or registration failed");
        }

        const auto sid = session_store().createSession(
            *userUuid, AuthenticatedHandlerBase::kSessionTtlSeconds);
        SetSessionCookie(request, sid);
        SetJsonContentType(request);

        return http::Created(
            request,
            json(AuthResponse{*userUuid, "User registered successfully"}).dump());
    } catch (const std::exception& e) {
        return http::BadRequest(request, e.what());
    }
}

std::string AuthHandler::Login(
    const userver::server::http::HttpRequest& request) const {
    try {
        auto data = json::parse(request.RequestBody());
        auto req = data.get<LoginRequest>();

        auto userUuid = auth_service_.loginUser(req.email, req.password);
        if (!userUuid) {
            return http::Unauthorized(request, "Invalid credentials");
        }

        const auto sid = session_store().createSession(
            *userUuid, AuthenticatedHandlerBase::kSessionTtlSeconds);
        SetSessionCookie(request, sid);
        SetJsonContentType(request);

        return http::Ok(
            request, json(AuthResponse{*userUuid, "Login successful"}).dump());
    } catch (const std::exception& e) {
        return http::BadRequest(request, e.what());
    }
}

std::string AuthHandler::Logout(
    const userver::server::http::HttpRequest& request) const {
    try {
        if (auto sid = GetSessionId(request)) {
            session_store().destroySession(*sid);
        }

        ClearSessionCookie(request);
        SetJsonContentType(request);

        return http::Ok(request,
                        json(AuthResponse{"", "Logout successfully"}).dump());
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

std::string AuthHandler::Me(
    const userver::server::http::HttpRequest& request) const {
    try {
        auto userUuid = GetCurrentUserUuid(request);
        if (!userUuid) {
            return http::Unauthorized(request);
        }

        UserRepository userRepo(auth_service_.getCluster());
        auto user = userRepo.getByUUID(*userUuid);
        if (!user) {
            return http::NotFound(request, "User not found");
        }

        SetJsonContentType(request);
        return http::Ok(
            request,
            json(MeResponse{user->user_uuid, user->email, user->created_at})
                .dump());
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

std::string AuthHandler::ChangePassword(
    const userver::server::http::HttpRequest& request) const {
    try {
        auto userUuid = GetCurrentUserUuid(request);
        if (!userUuid) {
            return http::Unauthorized(request);
        }

        auto data = json::parse(request.RequestBody());
        auto req = data.get<ChangePasswordRequest>();

        if (req.current_password.empty() || req.new_password.empty()) {
            return http::BadRequest(
                request,
                "Both current_password and new_password are required");
        }

        const auto result = auth_service_.changePassword(
            *userUuid, req.current_password, req.new_password);

        switch (result) {
            case AuthService::ChangePasswordResult::kOk: {
                session_store().destroyAllSessionsForUser(*userUuid);

                const auto newSid = session_store().createSession(
                    *userUuid, AuthenticatedHandlerBase::kSessionTtlSeconds);
                SetSessionCookie(request, newSid);
                SetJsonContentType(request);
                return http::Ok(
                    request,
                    json(AuthResponse{*userUuid, "Password changed"}).dump());
            }
            case AuthService::ChangePasswordResult::kWrongCurrentPassword:
                return http::Unauthorized(
                    request, "current_password is incorrect");
            case AuthService::ChangePasswordResult::kInvalidNewPassword:
                return http::BadRequest(request,
                                        "new_password must not be blank");
            case AuthService::ChangePasswordResult::kUserNotFound:
                return http::NotFound(request, "User not found");
        }
        return http::InternalError(request);
    } catch (const std::exception& e) {
        return http::BadRequest(request, e.what());
    }
}

std::string AuthHandler::UpdateUser(
    const userver::server::http::HttpRequest& request) const {
    try {
        auto userUuid = GetCurrentUserUuid(request);
        if (!userUuid) {
            return http::Unauthorized(request);
        }

        auto body = json::parse(request.RequestBody());

        if (body.contains("password")) {
            return http::BadRequest(
                request,
                "Password change is not allowed here; use POST /api/auth/change-password");
        }

        std::optional<std::string> email;
        if (body.contains("email") && !body["email"].is_null()) {
            email = body["email"].get<std::string>();
        }

        if (!email) {
            return http::BadRequest(request,
                                    "At least one field must be provided");
        }

        UserRepository userRepo(auth_service_.getCluster());

        auto existing = userRepo.getByEmail(*email);
        if (existing && existing->user_uuid != *userUuid) {
            return http::SetStatus(
                request, userver::server::http::HttpStatus::kConflict,
                "Email already in use");
        }

        userRepo.updateUser(*userUuid, email, std::nullopt);

        auto updated = userRepo.getByUUID(*userUuid);
        if (!updated) {
            return http::NotFound(request, "User not found");
        }

        json response = {
            {"user_uuid", updated->user_uuid},
            {"email", updated->email},
            {"message", "User updated successfully"},
        };

        SetJsonContentType(request);
        return http::Ok(request, response.dump());
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

std::string AuthHandler::DeleteUser(
    const userver::server::http::HttpRequest& request) const {
    try {
        auto userUuid = GetCurrentUserUuid(request);
        if (!userUuid) {
            return http::Unauthorized(request);
        }

        UserRepository userRepo(auth_service_.getCluster());
        userRepo.deleteUser(*userUuid);

        session_store().destroyAllSessionsForUser(*userUuid);

        ClearSessionCookie(request);
        return http::NoContent(request);
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

}  // namespace tracker_api
