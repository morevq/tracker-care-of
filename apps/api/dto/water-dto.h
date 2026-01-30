#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace tracker_api {

	struct WaterResponse {
		int id;
		std::string lastWater;
		int frequency;
		std::string frequencyMeasure;
	};
	
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WaterResponse, id, lastWater, frequency, frequencyMeasure)

}