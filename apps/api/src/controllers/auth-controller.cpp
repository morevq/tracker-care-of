#include "controllers/auth-controller.h"
#include <nlohmann/json.hpp>

#include "middleware/auth-middleware.h"
#include "tracker_db/repositories/user-repository.h"

#include <tracker_session/cookie.h>

#ifdef DELETE
#undef DELETE
#endif

using json = nlohmann::json;

namespace tracker_api {

    static constexpr int kSessionTtlSeconds = 86400;
	static constexpr bool kCookieSecure = false; // DO NOT FORGOT SET true in PROD (using HTTPS)
    static constexpr const char* kSameSite = "Strict";
    static constexpr const char* kCookieName = "__Host-session";

    AuthController::AuthController(AuthService& authService,
        std::shared_ptr<tracker_session::SessionStore> sessionStore)
        : authService(authService), sessionStore_(std::move(sessionStore)) {
    }

    void AuthController::registerRoutes(crow::SimpleApp& app) {
        CROW_ROUTE(app, "/api/auth/register")
            .methods(crow::HTTPMethod::POST)
            ([this](const crow::request& req) {
            return this->registerUser(req);
                });

        CROW_ROUTE(app, "/api/auth/login")
            .methods(crow::HTTPMethod::POST)
            ([this](const crow::request& req) {
            return this->loginUser(req);
                });

        CROW_ROUTE(app, "/api/auth/logout")
            .methods(crow::HTTPMethod::POST)
            ([this](const crow::request& req) {
            return this->logoutUser(req);
                });

        CROW_ROUTE(app, "/api/auth/user")
            .methods(crow::HTTPMethod::DELETE)
            ([this](const crow::request& req) {
            return this->deleteUser(req);
                });
    }

    crow::response AuthController::registerUser(const crow::request& req) {
        try {
            auto requestData = json::parse(req.body);
            RegisterRequest registerRequest = requestData.get<RegisterRequest>();

            auto userUuid = authService.registerUser(registerRequest.email, registerRequest.password);
            if (!userUuid) {
                return crow::response(400, "User already exists or registration failed");
            }

            std::string sid = sessionStore_->createSession(*userUuid, kSessionTtlSeconds);

            crow::response res(201);
            res.add_header("Set-Cookie",
                tracker_session::cookie::buildSetCookie(
                    kCookieName, sid, kSessionTtlSeconds,
                    /*httpOnly=*/true,
                    /*secure=*/kCookieSecure,
                    /*sameSite=*/kSameSite,
                    /*path=*/"/"
                )
            );

            res.write(json(AuthResponse{ *userUuid, "User registered successfully" }).dump());
            res.add_header("Content-Type", "application/json");
            return res;
        }
        catch (const std::exception& e) {
            return crow::response(400, "Invalid request: " + std::string(e.what()));
        }
    }

    crow::response AuthController::loginUser(const crow::request& req) {
        try {
            auto requestData = json::parse(req.body);
            LoginRequest loginRequest = requestData.get<LoginRequest>();

            auto userUuid = authService.loginUser(loginRequest.email, loginRequest.password);
            if (!userUuid) {
                return crow::response(401, "Invalid credentials");
            }

            std::string sid = sessionStore_->createSession(*userUuid, kSessionTtlSeconds);

            crow::response res(200);
            res.add_header("Set-Cookie",
                tracker_session::cookie::buildSetCookie(
                    kCookieName, sid, kSessionTtlSeconds,
                    true, kCookieSecure, kSameSite, "/"
                )
            );

            res.write(json(AuthResponse{ *userUuid, "Login successful" }).dump());
            res.add_header("Content-Type", "application/json");
            return res;
        }
        catch (const std::exception& e) {
            return crow::response(400, "Invalid request: " + std::string(e.what()));
        }
    }

    crow::response AuthController::logoutUser(const crow::request& req) {
        try {
            auto sid = AuthMiddleware::getSessionIdFromCookie(req);
            if (sid) {
                sessionStore_->destroySession(*sid);
            }

            crow::response res(200);
            res.add_header("Set-Cookie",
                tracker_session::cookie::buildDeleteCookie(
                    kCookieName, /*httpOnly=*/true, /*secure=*/kCookieSecure, kSameSite, "/"
                )
            );

            res.write(json(AuthResponse{ "", "Logged out successfully" }).dump());
            res.add_header("Content-Type", "application/json");
            return res;
        }
        catch (const std::exception& e) {
            return crow::response(500, "Internal server error: " + std::string(e.what()));
        }
    }

    crow::response AuthController::deleteUser(const crow::request& req) {
        try {
            auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
            if (!userUuid) {
                return crow::response(401, "Unauthorized");
            }

            UserRepository userRepo(authService.getConnection());
            userRepo.deleteUser(*userUuid);

            auto sid = AuthMiddleware::getSessionIdFromCookie(req);
            if (sid) {
                sessionStore_->destroySession(*sid);
            }

            crow::response res(204);
            res.add_header("Set-Cookie",
                tracker_session::cookie::buildDeleteCookie(
                    kCookieName, true, kCookieSecure, kSameSite, "/"
                )
            );
            return res;
        }
        catch (const std::exception& e) {
            return crow::response(500, "Internal server error: " + std::string(e.what()));
        }
    }

}