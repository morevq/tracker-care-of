#include <iostream>
#include "tracker_db/repositories/user-repository.h"

UserRepository::UserRepository(db_utils::PGconnPtr connection) : connection(connection) {}

std::optional<User> UserRepository::getByEmail(const std::string& email) {
	const char* params[1] = { email.c_str() };
	const char* query = "SELECT user_uuid, email, password_hash FROM users WHERE email = $1 AND is_deleted = FALSE;";

	auto res = db_utils::make_pgresult(PQexecParams(
		connection.get(),
		query,
		1,
		nullptr,
		params,
		nullptr,
		nullptr,
		0
	));

	if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
		return std::nullopt;
	}
	if (PQntuples(res.get()) == 0) {
		return std::nullopt;
	}

	User user;
	user.user_uuid = PQgetvalue(res.get(), 0, 0);
	user.email = PQgetvalue(res.get(), 0, 1);
	user.password_hash = PQgetvalue(res.get(), 0, 2);

	return user;
}

std::optional<User> UserRepository::getByUUID(const std::string& user_uuid) {
	const char* params[1] = { user_uuid.c_str() };
	const char* query = "SELECT user_uuid, email, password_hash FROM users WHERE user_uuid = $1 AND is_deleted = FALSE;";

	auto res = db_utils::make_pgresult(PQexecParams(
		connection.get(),
		query,
		1,
		nullptr,
		params,
		nullptr,
		nullptr,
		0
	));

	if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
		return std::nullopt;
	}
	if (PQntuples(res.get()) == 0) {
		return std::nullopt;
	}

	User user;
	user.user_uuid = PQgetvalue(res.get(), 0, 0);
	user.email = PQgetvalue(res.get(), 0, 1);
	user.password_hash = PQgetvalue(res.get(), 0, 2);

	return user;
}

std::string UserRepository::createUser(const std::string& email, const std::string& passwordHash) {
	const char* params[2] = { email.c_str(), passwordHash.c_str() };
	const char* query =
		"INSERT INTO users (email, password_hash) VALUES ($1, $2) RETURNING user_uuid;";

	auto res = db_utils::make_pgresult(PQexecParams(
		connection.get(),
		query,
		2,
		nullptr,
		params,
		nullptr,
		nullptr,
		0
	));

	if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
		std::cerr << "Error inserting user: " << PQerrorMessage(connection.get()) << std::endl;
		return "";
	}

	std::string user_uuid;
	if (PQntuples(res.get()) > 0) {
		user_uuid = PQgetvalue(res.get(), 0, 0);
	}
	return user_uuid;
}

void UserRepository::deleteUser(const std::string& user_uuid) {
	const char* params[] = { user_uuid.c_str() };
	const char* query = "UPDATE users SET is_deleted = TRUE WHERE user_uuid = $1;";

	auto res = db_utils::make_pgresult(PQexecParams(
		connection.get(),
		query,
		1,
		nullptr,
		params,
		nullptr,
		nullptr,
		0
	));

	if (PQresultStatus(res.get()) != PGRES_COMMAND_OK) {
		std::cerr << "Error deleting user: " << PQerrorMessage(connection.get()) << std::endl;
	}
}

void UserRepository::updateUser(const std::string& user_uuid, 
                                const std::optional<std::string>& email, 
                                const std::optional<std::string>& password_hash) {
	if (!email.has_value() && !password_hash.has_value()) {
		return;
	}

	std::string query = "UPDATE users SET ";
	std::vector<std::string> setClauses;
	std::vector<const char*> params;
	int paramIndex = 1;

	if (email.has_value()) {
		setClauses.push_back("email = $" + std::to_string(paramIndex++));
		params.push_back(email->c_str());
	}

	if (password_hash.has_value()) {
		setClauses.push_back("password_hash = $" + std::to_string(paramIndex++));
		params.push_back(password_hash->c_str());
	}

	query += setClauses[0];
	for (size_t i = 1; i < setClauses.size(); ++i) {
		query += ", " + setClauses[i];
	}

	query += " WHERE user_uuid = $" + std::to_string(paramIndex) + " AND is_deleted = FALSE;";
	params.push_back(user_uuid.c_str());

	auto res = db_utils::make_pgresult(PQexecParams(
		connection.get(),
		query.c_str(),
		static_cast<int>(params.size()),
		nullptr,
		params.data(),
		nullptr,
		nullptr,
		0
	));

	if (PQresultStatus(res.get()) != PGRES_COMMAND_OK) {
		std::cerr << "Error updating user: " << PQerrorMessage(connection.get()) << std::endl;
	}
}