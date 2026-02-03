#pragma once

#include <string>
#include <optional>
#include <libpq-fe.h>

#include "tracker/models/user.h"

class UserRepository {
private:
	PGconn* connection;

public:
	UserRepository(PGconn* connection);
			
	std::optional<User> getByEmail(const std::string& email);
	std::optional<User> getByUUID(const std::string& user_uuid);

	std::string createUser(const std::string& email, const std::string& passwordHash);

	void deleteUser(const std::string& user_uuid);
};