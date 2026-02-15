#pragma once
#include <crow.h>
#include <string>

namespace tracker_api {
    namespace responses {

        namespace messages {
            inline const std::string UNAUTHORIZED = "Unauthorized";
            inline const std::string FORBIDDEN = "Forbidden: Access denied";
            inline const std::string NOT_FOUND = "Not found";
            inline const std::string INTERNAL_ERROR = "Internal server error";
            inline const std::string INVALID_JSON = "Invalid JSON";
        }

        inline crow::response error(crow::status status, const std::string& message) {
            return crow::response(status, message);
        }

        inline crow::response unauthorized(const std::string& msg = messages::UNAUTHORIZED) {
            return error(crow::status::UNAUTHORIZED, msg);
        }

        inline crow::response forbidden(const std::string& msg = messages::FORBIDDEN) {
            return error(crow::status::FORBIDDEN, msg);
        }

        inline crow::response notFound(const std::string& msg = messages::NOT_FOUND) {
            return error(crow::status::NOT_FOUND, msg);
        }

        inline crow::response badRequest(const std::string& msg) {
            return error(crow::status::BAD_REQUEST, msg);
        }

        inline crow::response internalError(const std::string& msg = messages::INTERNAL_ERROR) {
            return error(crow::status::INTERNAL_SERVER_ERROR, msg);
        }

        inline crow::response ok(const std::string& msg) {
            return crow::response(crow::status::OK, msg);
        }

        inline crow::response ok(crow::json::wvalue&& json) {
            return crow::response(crow::status::OK, std::move(json));
        }

        inline crow::response created(const std::string& msg) {
            return crow::response(crow::status::CREATED, msg);
        }

        inline crow::response noContent() {
            return crow::response(crow::status::NO_CONTENT);
        }

    }
}
