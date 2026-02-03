#include "controllers/auth-controller.h"
#include <nlohmann/json.hpp>

#include "middleware/auth-middleware.h"
#include "tracker_db/repositories/user-repository.h"

#ifdef DELETE
#undef DELETE
#endif

using json = nlohmann::json;

namespace tracker_api {

	 AuthController::AuthController(AuthService & authService) : authService(authService) {}

	void AuthController::registerRoutes(crow::SimpleApp & app) {
		CROW_ROUTE(app, "/api/auth/register")
			.methods(crow::HTTPMethod::POST)
			([this](const crow::request& req) {
				return this->registerUser(req);
		});

		CROW_ROUTE(app, "/api/auth/login")
			.methods(crow::HTTPMethod::POST)
			([this, &app](const crow::request& req) {
				return this->loginUser(req);
		});

	CROW_ROUTE(app, "/api/auth/logout")
		.methods(crow::HTTPMethod::POST)
		([this](const crow::request&) {
		crow::response res(200);
			res.add_header("Set-Cookie", "session_uuid=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
			res.write(json(AuthResponse{ "", "Logged out successfully" }).dump());
			res.add_header("Content-Type", "application/json");
				return res;
		});

		CROW_ROUTE(app, "/api/auth/user")
			.methods(crow::HTTPMethod::DELETE)
			([this](const crow::request& req) {
				return this->deleteUser(req);
		});
	}

	crow::response AuthController::registerUser(const crow::request & req) {
		try {
		auto requestData = json::parse(req.body);
		RegisterRequest registerRequest = requestData.get<RegisterRequest>();

		auto userUuid = authService.registerUser(registerRequest.email, registerRequest.password);

			if (!userUuid) {
				return crow::response(400, "User already exists or registration failed");
			}

			crow::response res(201);
			res.add_header("Set-Cookie", "session_uuid=" + *userUuid + 
				"; Path=/; Max-Age=86400; HttpOnly; SameSite=Strict");
			res.write(json(AuthResponse{ *userUuid, "User registered successfully" }).dump());
			res.add_header("Content-Type", "application/json");
			return res;
		}
		catch (const std::exception& e) {
			return crow::response(400, "Invalid request: " + std::string(e.what()));
		}
	}

	crow::response AuthController::loginUser(const crow::request & req) {
		try {
			auto requestData = json::parse(req.body);
			LoginRequest loginRequest = requestData.get<LoginRequest>();

		auto userUuid = authService.loginUser(loginRequest.email, loginRequest.password);

		if (!userUuid) {
			return crow::response(401, "Invalid credentials");
		}

			crow::response res(200);
			res.add_header("Set-Cookie", "session_uuid=" + *userUuid + 
				"; Path=/; Max-Age=86400; HttpOnly; SameSite=Strict");
			res.write(json(AuthResponse{ *userUuid, "Login successful" }).dump());
			res.add_header("Content-Type", "application/json");
			return res;
		}
		catch (const std::exception& e) {
			return crow::response(400, "Invalid request: " + std::string(e.what()));
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

			crow::response res(204);
			res.add_header("Set-Cookie", "session_uuid=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
			return res;
		}
		catch (const std::exception& e) {
			return crow::response(500, "Internal server error: " + std::string(e.what()));
		}
	}
}