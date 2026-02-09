#include "controllers/patient-controller.h"
#include "middleware/auth-middleware.h"
#include <nlohmann/json.hpp>

#ifdef DELETE
#undef DELETE
#endif

using json = nlohmann::json;

namespace tracker_api {

	PatientController::PatientController(PatientRepository& patientRepo) : patientRepo(patientRepo) {}

	void PatientController::registerRoutes(crow::SimpleApp& app) {
		CROW_ROUTE(app, "/api/patients")
			.methods(crow::HTTPMethod::POST)
			([this](const crow::request& req) {
				return this->createPatient(req);
		});

		CROW_ROUTE(app, "/api/patients")
			.methods(crow::HTTPMethod::GET)
			([this](const crow::request& req) {
				return this->getPatients(req);
		});

		CROW_ROUTE(app, "/api/patients/<int>")
			.methods(crow::HTTPMethod::GET)
			([this](const crow::request& req, int id) {
				return this->getPatientById(req, id);
		});

		CROW_ROUTE(app, "/api/patients/<int>")
			.methods(crow::HTTPMethod::DELETE)
			([this](const crow::request& req, int id) {
				return this->deletePatient(req, id);
		});

		CROW_ROUTE(app, "/api/patients/<int>")
			.methods("PATCH"_method)
			([this](const crow::request& req, int id) {
			return updatePatient(req, id);
		});
	}

	crow::response PatientController::createPatient(const crow::request& req) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return crow::response(401, "Unauthorized");
			}

			auto requestData = json::parse(req.body);
			CreatePatientRequest createReq = requestData.get<CreatePatientRequest>();

			patientRepo.createPatient(*userUuid, createReq.name, createReq.birth_date);
			
			return crow::response(201, "Patient created successfully");
		}
		catch (const std::exception& e) {
			return crow::response(400, "Invalid request: " + std::string(e.what()));
		}
	}

	crow::response PatientController::getPatients(const crow::request& req) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return crow::response(400, "Unauthorized");
			}

			std::vector<Patient> patients = patientRepo.getByUserUUID(*userUuid);

			std::vector<PatientResponse> response;
			for (const Patient& patient: patients) {
				response.push_back(PatientResponse{
					patient.id_patient,
					patient.name,
					patient.birth_date
				});
			}

			return crow::response(200, json(response).dump());
		}
		catch (const std::exception& e) {
			return crow::response(500, "Internal server error: " + std::string(e.what()));
		}
	}

	crow::response PatientController::getPatientById(const crow::request& req, int id) {
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

			PatientResponse response{
				patient->id_patient,
				patient->name,
				patient->birth_date,
				patient->created_at
			};

			return crow::response(200, json(response).dump());
		}
		catch (const std::exception& e) {
			return crow::response(500, "Internal server error: " + std::string(e.what()));
		}
	}

	crow::response PatientController::updatePatient(const crow::request& req, int id) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return crow::response(401, "Unauthorized");
			}

			auto patient = patientRepo.getByID(id);
			if (!patient.has_value()) {
				return crow::response(404, "Patient not found");
			}

			if (patient->user_uuid != *userUuid) {
				return crow::response(403, "Forbidden: Access denied");
			}

			auto body = crow::json::load(req.body);
			if (!body) {
				return crow::response(400, "Invalid JSON");
			}

			std::optional<std::string> name;
			std::optional<std::string> birth_date;

			if (body.has("name")) {
				name = body["name"].s();
			}

			if (body.has("birth_date")) {
				birth_date = body["birth_date"].s();
			}

			if (!name.has_value() && !birth_date.has_value()) {
				return crow::response(400, "At least one field must be provided");
			}

			patientRepo.updatePatient(id, name, birth_date);

			auto updated = patientRepo.getByID(id);
			crow::json::wvalue response;
			response["id_patient"] = updated->id_patient;
			response["name"] = updated->name;
			if (updated->birth_date.has_value()) {
				response["birth_date"] = *updated->birth_date;
			}

			return crow::response(200, response);
		}
		catch (const std::exception& e) {
			return crow::response(500, "Internal server error: " + std::string(e.what()));
		}
	}


	crow::response PatientController::deletePatient(const crow::request& req, int id) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return crow::response(401, "Unauthorized");
			}

			auto patient = patientRepo.getByID(id);
			if (!patient || patient->user_uuid != *userUuid) {
				return crow::response(403, "Forbidden: Access denied");
			}

			patientRepo.deletePatient(id);
			return crow::response(204);
		}
		catch (const std::exception& e) {
			return crow::response(500, "Internal server error: " + std::string(e.what()));
		}
	}
}