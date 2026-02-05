#include <iostream>
#include <cstdlib> // for std::getenv

#include "tracker_common/env-parser.h"

std::unordered_map<std::string, std::string> load_env(const std::string& path) {
	std::unordered_map<std::string, std::string> env;
	std::ifstream file(path);

	if (!file.is_open()) {
		// Just return empty, assuming env vars set
		return env;
	}

	std::string line;
	while (std::getline(file, line)) {
		size_t delimiter_pos = line.find('=');
		if (delimiter_pos != std::string::npos) {
			std::string key = line.substr(0, delimiter_pos);
			std::string value = line.substr(delimiter_pos + 1);
			env[key] = value;
		}
	}

	return env;
}

std::string get_env_var(const std::unordered_map<std::string, std::string>& env, const std::string& key, const std::string& default_val) {
	if (const char* val = std::getenv(key.c_str())) {
		return std::string(val);
	}
	auto it = env.find(key);
	if (it != env.end()) {
		return it->second;
	}
	return default_val;
}
