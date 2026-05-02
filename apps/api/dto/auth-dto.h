#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace tracker_api {

	struct RegisterRequest {
		std::string email;
		std::string password;
	};

	struct LoginRequest {
		std::string email;
		std::string password;
	};

	struct ChangePasswordRequest {
		std::string current_password;
		std::string new_password;
	};

	struct AuthResponse {
		std::string user_uuid;
		std::string message;
	};

	struct MeResponse {
		std::string user_uuid;
		std::string email;
		std::string created_at;
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RegisterRequest, email, password)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LoginRequest, email, password)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ChangePasswordRequest, current_password, new_password)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AuthResponse, user_uuid, message)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MeResponse, user_uuid, email, created_at)

};
