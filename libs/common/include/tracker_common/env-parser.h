#pragma once

#include <fstream>
#include <string>
#include <unordered_map>


std::unordered_map<std::string, std::string> load_env(const std::string& path);
std::string get_env_var(const std::unordered_map<std::string, std::string>& env, const std::string& key, const std::string& default_val = "");

