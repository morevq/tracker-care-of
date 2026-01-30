#pragma once
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace tracker_api {

	struct AnamnesisResponse {
		int id;
		std::string description;
		std::optional<std::string> photo_url;
		std::string date;
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnamnesisResponse, id, description, photo_url, date)

};