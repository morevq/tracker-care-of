#pragma once

#include <libpq-fe.h>
#include <optional>
#include <string>

class AuthService {
private:
	PGconn* connection;

public:
	AuthService(PGconn* connection);

	std::optional<std::string> registerUser(const std::string& email, const std::string& password);
	std::optional<std::string> loginUser(const std::string& email, const std::string& password);
};