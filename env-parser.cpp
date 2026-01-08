#include <iostream>

#include "env-parser.h"

std::unordered_map<std::string, std::string> load_env(const std::string& path) {
	std::unordered_map<std::string, std::string> env;
	std::ifstream file(path);

	if (!file.is_open()) {
		throw std::runtime_error("Could not open .env file");
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
