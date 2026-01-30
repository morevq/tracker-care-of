#pragma once
#include <crow.h>
#include <string>
#include <optional>

namespace tracker_api {
	
	class AuthMiddleware {
	private:
		static std::string parseCookie(const std::string& cookieHeader, const std::string& key);

	public:
		static std::optional<std::string> getUserUuidFromCookie(const crow::request& req);
		static bool isAuthenticated(const crow::request& req);
	};

}