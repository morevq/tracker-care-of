#pragma once
#include <crow.h>
#include "tracker_db/repositories/water-repository.h"
#include "../dto/water-dto.h"

namespace tracker_api {

	class WaterController {
	private:
		WaterRepository& waterRepo;

		crow::response getWaterData(const crow::request& req);

	public:
		WaterController(WaterRepository& waterRepo);

		void registerRoutes(crow::SimpleApp& app);
	};

}