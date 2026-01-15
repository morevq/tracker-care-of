#include "crypto/password-hasher.h"

#include <argon2.h>
#include <iostream>
#include <random>
#include <vector>

#include "env/env-parser.h"

struct Argon2Params {
	std::size_t memoryCost;
	std::size_t timeCost;
	std::size_t parallelism;
	std::size_t hashLength;
	std::size_t saltLength;
};

const Argon2Params& getParams() {
	Argon2Params params;
	static bool initialized = false;

	try {
		if (!initialized) {
			std::unordered_map<std::string, std::string> env = load_env("../../.env");
			params.memoryCost = std::stoul(env["ARGON2_MEMORY_COST"]);
			params.timeCost = std::stoul(env["ARGON2_TIME_COST"]);
			params.parallelism = std::stoul(env["ARGON2_PARALLELISM"]);
			params.hashLength = std::stoul(env["ARGON2_HASH_LENGTH"]);
			params.saltLength = std::stoul(env["ARGON2_SALT_LENGTH"]);
			initialized = true;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Failed to load Argon2 parameters from .env: " + std::string(e.what());
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
	std::vector<char> salt = generateSalt(params.saltLength);
	char hashBuffer[128];

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
		std::cerr << "Error hashing password: " + std::string(argon2_error_message(rc));
	}

	return std::string(hashBuffer);
}