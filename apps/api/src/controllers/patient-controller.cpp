#include "controllers/patient-controller.h"
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
			auto requestData = json::parse(req.body);
			CreatePatientRequest createRequest = requestData.get<CreatePatientRequest>();

			patientRepo.createPatient(createRequest.userUuid, createRequest.name, createRequest.birth_date);
			
			return crow::response(201, "Patient created successfully");
		}
		catch (const std::exception& e) {
			return crow::response(400, "Invalid request: " + std::string(e.what()));
		}
	}

	crow::response PatientController::getPatients(const crow::request& req) {
		try {
			auto userUuid = req.url_params.get("user_uuid");
			if (!userUuid) {
				return crow::response(400, "Missing user_uuid parameter");
			}

			std::vector<Patient> patients = patientRepo.getByUserUUID(userUuid);

			std::vector<PatientResponse> response;
			for (const Patient& patient: patients) {
				response.push_back(PatientResponse{
					patient.id_patient,
					patient.user_uuid,
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
			auto patient = patientRepo.getByID(id);

			if (!patient) {
				return crow::response(404, "Patient not found");
			}

			PatientResponse response{
				patient->id_patient,
				patient->user_uuid,
				patient->name,
				patient->birth_date
			};

			return crow::response(200, json(response).dump());
		}
		catch (const std::exception& e) {
			return crow::response(500, "Internal server error: " + std::string(e.what()));
		}
	}
}