#pragma once

#include <string>
#include <optional>
#include <libpq-fe.h>
#include <memory>
#include <tracker_db/db-utils.h>

#include "tracker/models/user.h"

class UserRepository {
private:
	db_utils::PGconnPtr connection;

public:
	UserRepository(db_utils::PGconnPtr connection);
			
	std::optional<User> getByEmail(const std::string& email);
	std::optional<User> getByUUID(const std::string& user_uuid);

	std::string createUser(const std::string& email, const std::string& passwordHash);

	void deleteUser(const std::string& user_uuid);
};