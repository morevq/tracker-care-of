#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace tracker_api {

	struct WaterIntakeResponse {
		int id;
		std::string lastWater;
		int frequency;
		std::string frequencyMeasure;
	};
	
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WaterIntakeResponse, id, lastWater, frequency, frequencyMeasure)

};