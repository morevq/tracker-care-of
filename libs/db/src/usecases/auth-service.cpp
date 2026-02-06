#include "tracker_db/usecases/auth-service.h"

#include <algorithm>
#include <cctype>

#include "tracker_crypto/password-hasher.h"
#include "tracker_db/repositories/user-repository.h"

bool isBlank(const std::string& str) {
	return std::all_of(str.begin(), str.end(), isspace);
}

AuthService::AuthService(db_utils::PGconnPtr connection) : connection(connection) {}

std::optional<std::string> AuthService::registerUser(const std::string& email, const std::string& password) {
	if (email.empty() || password.empty() || isBlank(email) || isBlank(password)) {
		return std::nullopt;
	}

	UserRepository userRepo(connection);

	if (userRepo.getByEmail(email).has_value()) {
		return std::nullopt;
	}

	const std::string passwordHash = PasswordHasher::hashPassword(password);
	return userRepo.createUser(email, passwordHash);
}

std::optional<std::string> AuthService::loginUser(const std::string& email, const std::string& password) {
	if (email.empty() || password.empty() || isBlank(email) || isBlank(password)) {
		return std::nullopt;
	}

	UserRepository userRepo(connection);
	const auto userOpt = userRepo.getByEmail(email);

	if (!userOpt.has_value()) {
		return std::nullopt;
	}

	const User& user = userOpt.value();
	if (!PasswordHasher::verifyPassword(password, user.password_hash)) {
		return std::nullopt;
	}
	return user.user_uuid;
}

db_utils::PGconnPtr AuthService::getConnection() const { return connection; }
