#include "controllers/water-controller.h"
#include "middleware/auth-middleware.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tracker_api {

	WaterController::WaterController(WaterRepository& waterRepo)
		: waterRepo(waterRepo) {}

	void WaterController::registerRoutes(crow::SimpleApp& app) {
		CROW_ROUTE(app, "/api/water")
			.methods(crow::HTTPMethod::GET)
			([this](const crow::request& req) {
				return this->getWaterData(req);
			});
	}

	crow::response WaterController::getWaterData(const crow::request& req) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return crow::response(401, "Unauthorized");
			}

			auto waterRecords = waterRepo.getByUserUUID(*userUuid);
			
			std::vector<WaterResponse> response;
			for (const auto& water : waterRecords) {
				response.push_back({
					water.idPatient,
					water.lastWater,
					water.frequency,
					water.frequencyMeasure
				});
			}

			return crow::response(200, json(response).dump());
		}
		catch (const std::exception& e) {
			return crow::response(500, "Internal server error: " + std::string(e.what()));
		}
	}

}