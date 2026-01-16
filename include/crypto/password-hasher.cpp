#include "crypto/password-hasher.h"

#include <argon2.h>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "env/env-parser.h"

struct Argon2Params {
	std::size_t memoryCost;
	std::size_t timeCost;
	std::size_t parallelism;
	std::size_t hashLength;
	std::size_t saltLength;
};

static std::size_t getRequiredUlong(
	const std::unordered_map<std::string, std::string>& env,
	const char* key
) {
	const auto it = env.find(key);
	if (it == env.end() || it->second.empty()) {
		throw std::runtime_error(std::string("Missing .env key: ") + key);
	}
	return std::stoul(it->second);
}

const Argon2Params& getParams() {
	static Argon2Params params{};
	static bool initialized = false;

	if (!initialized) {
		const std::unordered_map<std::string, std::string> env = load_env(".env");

		// match actual .env keys
		params.timeCost = getRequiredUlong(env, "ARGON2_T_COST");
		params.memoryCost = getRequiredUlong(env, "ARGON2_M_COST_KIB");
		params.parallelism = getRequiredUlong(env, "ARGON2_PARALLELISM");
		params.saltLength = getRequiredUlong(env, "ARGON2_SALT_LEN");
		params.hashLength = getRequiredUlong(env, "ARGON2_HASH_LEN");

		initialized = true;
	}

	return params;
}

std::vector<char> generateSalt(std::size_t length) {
	std::vector<char> salt(length);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 255);

	for (std::size_t i = 0; i < length; ++i) {
		salt[i] = static_cast<char>(dis(gen));
	}

	return salt;
}

std::string PasswordHasher::hashPassword(const std::string& password) {
	const Argon2Params& params = getParams();

	const std::vector<char> salt = generateSalt(params.saltLength);
	char hashBuffer[512]{};

	const int rc = argon2id_hash_encoded(
		params.timeCost,
		params.memoryCost,
		params.parallelism,
		password.data(),
		password.size(),
		salt.data(),
		salt.size(),
		params.hashLength,
		hashBuffer,
		sizeof(hashBuffer)
	);

	if (rc != ARGON2_OK) {
		throw std::runtime_error(std::string("Error hashing password: ") + argon2_error_message(rc));
	}

	return std::string(hashBuffer);
}

bool PasswordHasher::verifyPassword(const std::string& password, const std::string& hashedPassword) {
	const int rc = argon2id_verify(
		hashedPassword.c_str(),
		password.data(),
		password.size()
	);

	if (rc == ARGON2_OK) {
		return true;
	}

	if (rc == ARGON2_VERIFY_MISMATCH) {
		return false;
	}

	std::cerr << "Error verifying password: " + std::string(argon2_error_message(rc));
	return false;
}