#include "controllers/water-controller.h"
#include "middleware/auth-middleware.h"
#include <nlohmann/json.hpp>

#ifdef DELETE
#undef DELETE
#endif

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

		CROW_ROUTE(app, "/api/water/<int>")
			.methods(crow::HTTPMethod::DELETE)
			([this](const crow::request& req, int id) {
			return this->deleteWater(req, id);
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

	crow::response WaterController::deleteWater(const crow::request& req, int id) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return crow::response(401, "Unauthorized");
			}

			waterRepo.deleteWater(id);
			return crow::response(204);
		}
		catch (const std::exception& e) {
			return crow::response(500, "Internal server error: " + std::string(e.what()));
		}
	}

}