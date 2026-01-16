#pragma once

#include <string>

struct User {
	std::string user_uuid;
	std::string email;
	std::string password_hash;
	std::string created_at;
};
