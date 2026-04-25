#pragma once

#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/http/http_status.hpp>

#include <string>

namespace tracker_api::http {

namespace messages {
inline const std::string kUnauthorized = "Unauthorized";
inline const std::string kForbidden = "Forbidden: Access denied";
inline const std::string kNotFound = "Not found";
inline const std::string kInternalError = "Internal server error";
inline const std::string kInvalidJson = "Invalid JSON";
}  // namespace messages

inline std::string SetStatus(
    const userver::server::http::HttpRequest& request,
    userver::server::http::HttpStatus status,
    std::string body = {}) {
    request.GetHttpResponse().SetStatus(status);
    return body;
}

inline std::string Ok(const userver::server::http::HttpRequest& request,
                      std::string body = {}) {
    return SetStatus(request, userver::server::http::HttpStatus::kOk,
                     std::move(body));
}

inline std::string Created(const userver::server::http::HttpRequest& request,
                           std::string body = {}) {
    return SetStatus(request, userver::server::http::HttpStatus::kCreated,
                     std::move(body));
}

inline std::string NoContent(const userver::server::http::HttpRequest& request) {
    return SetStatus(request, userver::server::http::HttpStatus::kNoContent);
}

inline std::string BadRequest(const userver::server::http::HttpRequest& request,
                              std::string message) {
    return SetStatus(request, userver::server::http::HttpStatus::kBadRequest,
                     std::move(message));
}

inline std::string Unauthorized(
    const userver::server::http::HttpRequest& request,
    std::string message = messages::kUnauthorized) {
    return SetStatus(request, userver::server::http::HttpStatus::kUnauthorized,
                     std::move(message));
}

inline std::string Forbidden(const userver::server::http::HttpRequest& request,
                             std::string message = messages::kForbidden) {
    return SetStatus(request, userver::server::http::HttpStatus::kForbidden,
                     std::move(message));
}

inline std::string NotFound(const userver::server::http::HttpRequest& request,
                            std::string message = messages::kNotFound) {
    return SetStatus(request, userver::server::http::HttpStatus::kNotFound,
                     std::move(message));
}

inline std::string InternalError(
    const userver::server::http::HttpRequest& request,
    std::string message = messages::kInternalError) {
    return SetStatus(request,
                     userver::server::http::HttpStatus::kInternalServerError,
                     std::move(message));
}

}  // namespace tracker_api::http
