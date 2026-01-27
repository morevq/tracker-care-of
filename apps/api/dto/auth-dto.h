#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace tracker_api {

	struct RegisterUserRequest {
		std::string email;
		std::string password;
	};

	struct LeginUserRequest {
		std::string email;
		std::string password;
	};

	struct AuthResponse {
		std::string message;
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RegisterRequest, email, password)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LoginRequest, email, password)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AuthResponse, user_uuid, message)

};