#include "controllers/patient-controller.h"
#include "middleware/auth-middleware.h"
#include <nlohmann/json.hpp>

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
					patient.birth_date,
					patient.created_at
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
}