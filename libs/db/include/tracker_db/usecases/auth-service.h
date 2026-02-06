#pragma once

#include <libpq-fe.h>
#include <optional>
#include <string>
#include <memory>
#include <tracker_db/db-utils.h>

class AuthService {
private:
	db_utils::PGconnPtr connection;

public:
	AuthService(db_utils::PGconnPtr connection);

	std::optional<std::string> registerUser(const std::string& email, const std::string& password);
	std::optional<std::string> loginUser(const std::string& email, const std::string& password);
	db_utils::PGconnPtr getConnection() const;
};