#include "controllers/water-controller.h"
#include "middleware/auth-middleware.h"
#include <nlohmann/json.hpp>

#ifdef DELETE
#undef DELETE
#endif

using json = nlohmann::json;

namespace tracker_api {

	WaterController::WaterController(WaterRepository& waterRepo, PatientRepository& patientRepo)
		: waterRepo(waterRepo), patientRepo(patientRepo) {}

	void WaterController::registerRoutes(crow::SimpleApp& app) {
		CROW_ROUTE(app, "/api/water")
			.methods(crow::HTTPMethod::GET)
			([this](const crow::request& req) {
			return this->getWaterData(req);
		});

		CROW_ROUTE(app, "/api/water/<int>")
			.methods(crow::HTTPMethod::GET)
			([this](const crow::request& req, int id) {
			return this->getWaterByPatientId(req, id);
		});

		CROW_ROUTE(app, "/api/water")
			.methods(crow::HTTPMethod::POST)
			([this](const crow::request& req) {
			return this->addWater(req);
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

	crow::response WaterController::getWaterByPatientId(const crow::request& req, int id) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return crow::response(401, "Unauthorized");
			}

			auto patient = patientRepo.getByID(id);
			if (!patient) {
				return crow::response(404, "Patient not found");
			}

			if (patient->user_uuid != *userUuid) {
				return crow::response(403, "Forbidden: Access denied");
			}

			auto water = waterRepo.getByPatientID(id);
			if (!water) {
				return crow::response(404, "Water record not found");
			}

			WaterResponse response{
				water->idPatient,
				water->lastWater,
				water->frequency,
				water->frequencyMeasure
			};

			return crow::response(200, json(response).dump());
		}
		catch (const std::exception& e) {
			return crow::response(500, "Internal server error: " + std::string(e.what()));
		}
	}

	crow::response WaterController::addWater(const crow::request& req) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return crow::response(401, "Unauthorized");
			}

			auto body = crow::json::load(req.body);
			if (!body) {
				return crow::response(400, "Invalid JSON");
			}

			if (!body.has("patient_id") || !body.has("last_water")) {
				return crow::response(400, "Missing required fields: patient_id and last_water");
			}

			int patientId = body["patient_id"].i();
			std::string lastWater = body["last_water"].s();

			auto patient = patientRepo.getByID(patientId);
			if (!patient || patient->user_uuid != *userUuid) {
				return crow::response(403, "Forbidden: Access denied");
			}

			bool success = waterRepo.addWater(patientId, lastWater);
			
			if (!success) {
				return crow::response(400, "Failed to add water record. Check that the date is not earlier than patient birth date.");
			}

			return crow::response(201, "Water record created successfully");
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

			auto patient = patientRepo.getByID(id);
			if (!patient || patient->user_uuid != *userUuid) {
				return crow::response(403, "Forbidden: Access denied");
			}

			waterRepo.deleteWater(id);
			return crow::response(204);
		}
		catch (const std::exception& e) {
			return crow::response(500, "Internal server error: " + std::string(e.what()));
		}
	}

}