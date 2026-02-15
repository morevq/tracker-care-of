#include "controllers/water-controller.h"
#include "middleware/auth-middleware.h"
#include <tracker_common/http-responses.h>
#include <nlohmann/json.hpp>

#ifdef DELETE
#undef DELETE
#endif

using json = nlohmann::json;

namespace tracker_api {

	WaterController::WaterController(WaterRepository& waterRepo, PatientRepository& patientRepo, crow::SimpleApp& app)
		: waterRepo(waterRepo), patientRepo(patientRepo), bp_("api/water") {
		setupRoutes();
		app.register_blueprint(bp_);
	}

	void WaterController::setupRoutes() {

		CROW_BP_ROUTE(bp_, "/")
			.methods(crow::HTTPMethod::GET)
			([this](const crow::request& req) {
			return this->getWaterData(req);
		});

		CROW_BP_ROUTE(bp_, "/<int>")
			.methods(crow::HTTPMethod::GET)
			([this](const crow::request& req, int id) {
			return this->getWaterByPatientId(req, id);
		});

		CROW_BP_ROUTE(bp_, "/")
			.methods(crow::HTTPMethod::POST)
			([this](const crow::request& req) {
			return this->addWater(req);
		});

		CROW_BP_ROUTE(bp_, "/<int>")
			.methods(crow::HTTPMethod::DELETE)
			([this](const crow::request& req, int id) {
			return this->deleteWater(req, id);
		});
	}

	crow::response WaterController::getWaterData(const crow::request& req) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return responses::unauthorized();
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

			return responses::ok(json(response).dump());
		}
		catch (const std::exception& e) {
			return responses::internalError(e.what());
		}
	}

	crow::response WaterController::getWaterByPatientId(const crow::request& req, int id) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return responses::unauthorized();
			}

			auto patient = patientRepo.getByID(id);
			if (!patient) {
				return responses::notFound("Patient not found");
			}

			if (patient->user_uuid != *userUuid) {
				return responses::forbidden();
			}

			auto water = waterRepo.getByPatientID(id);
			if (!water) {
				return responses::notFound("Water record not found");
			}

			WaterResponse response{
				water->idPatient,
				water->lastWater,
				water->frequency,
				water->frequencyMeasure
			};

			return responses::ok(json(response).dump());
		}
		catch (const std::exception& e) {
			return responses::internalError(e.what());
		}
	}

	crow::response WaterController::addWater(const crow::request& req) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return responses::unauthorized();
			}

			auto body = crow::json::load(req.body);
			if (!body) {
				return responses::badRequest(responses::messages::INVALID_JSON);
			}

			if (!body.has("patient_id") || !body.has("last_water")) {
				return responses::badRequest("Missing required fields: patient_id and last_water");
			}

			int patientId = body["patient_id"].i();
			std::string lastWater = body["last_water"].s();

			auto patient = patientRepo.getByID(patientId);
			if (!patient || patient->user_uuid != *userUuid) {
				return responses::forbidden();
			}

			bool success = waterRepo.addWater(patientId, lastWater);
			
			if (!success) {
				return responses::badRequest("Failed to add water record. Check that the date is not earlier than patient birth date.");
			}

			return responses::created("Water record created successfully");
		}
		catch (const std::exception& e) {
			return responses::internalError(e.what());
		}
	}

	crow::response WaterController::deleteWater(const crow::request& req, int id) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return responses::unauthorized();
			}

			auto patient = patientRepo.getByID(id);
			if (!patient || patient->user_uuid != *userUuid) {
				return responses::forbidden();
			}

			waterRepo.deleteWater(id);
			return responses::noContent();
		}
		catch (const std::exception& e) {
			return responses::internalError(e.what());
		}
	}

}