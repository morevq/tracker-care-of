#include <iostream>
#include "user-repository.h"

UserRepository::UserRepository(PGconn* connection) : connection(connection) {}

std::optional<User> UserRepository::getByEmail(const std::string& email) {
	const char* params[1] = { email.c_str() };
	const char* query = "SELECT user_uuid, email, password_hash FROM users WHERE email = $1";

	PGresult* res = PQexecParams(
		connection,
		query,
		1,
		nullptr,
		params,
		nullptr,
		nullptr,
		0
	);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		PQclear(res);
		return std::nullopt;
	}
	if (PQntuples(res) == 0) {
		PQclear(res);
		return std::nullopt;
	}

	User user;
	user.user_uuid = PQgetvalue(res, 0, 0);
	user.email = PQgetvalue(res, 0, 1);
	user.password_hash = PQgetvalue(res, 0, 2);

	PQclear(res);
	return user;
}

std::optional<User> UserRepository::getByUUID(const std::string& user_uuid) {
	const char* params[1] = { user_uuid.c_str() };
	const char* query = "SELECT user_uuid, email, password_hash FROM users WHERE user_uuid = $1";

	PGresult* res = PQexecParams(
		connection,
		query,
		1,
		nullptr,
		params,
		nullptr,
		nullptr,
		0
	);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		PQclear(res);
		return std::nullopt;
	}
	if (PQntuples(res) == 0) {
		PQclear(res);
		return std::nullopt;
	}

	User user;
	user.user_uuid = PQgetvalue(res, 0, 0);
	user.email = PQgetvalue(res, 0, 1);
	user.password_hash = PQgetvalue(res, 0, 2);

	PQclear(res);
	return user;
}

std::string UserRepository::createUser(const std::string& email, const std::string& passwordHash) {
	const char* params[2] = { email.c_str(), passwordHash.c_str() };
	const char* query =
		"INSERT INTO users (email, password_hash) VALUES ($1, $2) RETURNING user_uuid";

	PGresult* res = PQexecParams(
		connection,
		query,
		2,
		nullptr,
		params,
		nullptr,
		nullptr,
		0
	);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		std::cerr << "Error inserting user: " << PQerrorMessage(connection) << std::endl;
		PQclear(res);
		return "";
	}

	std::string user_uuid;
	if (PQntuples(res) > 0) {
		user_uuid = PQgetvalue(res, 0, 0);
	}
	PQclear(res);
	return user_uuid;
}