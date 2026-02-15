#include "controllers/patient-controller.h"
#include "middleware/auth-middleware.h"
#include <tracker_common/http-responses.h>
#include <nlohmann/json.hpp>

#ifdef DELETE
#undef DELETE
#endif

using json = nlohmann::json;

namespace tracker_api {

	PatientController::PatientController(PatientRepository& patientRepo) : patientRepo(patientRepo) {}

	crow::Blueprint PatientController::getBlueprint() {
		crow::Blueprint bp("api/patients");

		CROW_BP_ROUTE(bp, "/")
			.methods(crow::HTTPMethod::POST)
			([this](const crow::request& req) {
				return this->createPatient(req);
		});

		CROW_BP_ROUTE(bp, "/")
			.methods(crow::HTTPMethod::GET)
			([this](const crow::request& req) {
				return this->getPatients(req);
		});

		CROW_BP_ROUTE(bp, "/<int>")
			.methods(crow::HTTPMethod::GET)
			([this](const crow::request& req, int id) {
				return this->getPatientById(req, id);
		});

		CROW_BP_ROUTE(bp, "/<int>")
			.methods(crow::HTTPMethod::DELETE)
			([this](const crow::request& req, int id) {
				return this->deletePatient(req, id);
		});

		CROW_BP_ROUTE(bp, "/<int>")
			.methods("PATCH"_method)
			([this](const crow::request& req, int id) {
				return updatePatient(req, id);
		});

		return bp;
	}

	crow::response PatientController::createPatient(const crow::request& req) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return responses::unauthorized();
			}

			auto requestData = json::parse(req.body);
			CreatePatientRequest createReq = requestData.get<CreatePatientRequest>();

			patientRepo.createPatient(*userUuid, createReq.name, createReq.birth_date);
			
			return responses::created("Patient created successfully");
		}
		catch (const std::exception& e) {
			return responses::badRequest(e.what());
		}
	}

	crow::response PatientController::getPatients(const crow::request& req) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return responses::unauthorized();
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

			return responses::ok(json(response).dump());
		}
		catch (const std::exception& e) {
			return responses::internalError(e.what());
		}
	}

	crow::response PatientController::getPatientById(const crow::request& req, int id) {
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

			PatientResponse response{
				patient->id_patient,
				patient->name,
				patient->birth_date
			};

			return responses::ok(json(response).dump());
		}
		catch (const std::exception& e) {
			return responses::internalError(e.what());
		}
	}

	crow::response PatientController::updatePatient(const crow::request& req, int id) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return responses::unauthorized();
			}

			auto patient = patientRepo.getByID(id);
			if (!patient.has_value()) {
				return responses::notFound("Patient not found");
			}

			if (patient->user_uuid != *userUuid) {
				return responses::forbidden();
			}

			auto body = crow::json::load(req.body);
			if (!body) {
				return responses::badRequest(responses::messages::INVALID_JSON);
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
				return responses::badRequest("At least one field must be provided");
			}

			patientRepo.updatePatient(id, name, birth_date);

			auto updated = patientRepo.getByID(id);
			crow::json::wvalue response;
			response["id_patient"] = updated->id_patient;
			response["name"] = updated->name;
			if (updated->birth_date.has_value()) {
				response["birth_date"] = *updated->birth_date;
			}

			return responses::ok(std::move(response));
		}
		catch (const std::exception& e) {
			return responses::internalError(e.what());
		}
	}


	crow::response PatientController::deletePatient(const crow::request& req, int id) {
		try {
			auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
			if (!userUuid) {
				return responses::unauthorized();
			}

			auto patient = patientRepo.getByID(id);
			if (!patient || patient->user_uuid != *userUuid) {
				return responses::forbidden();
			}

			patientRepo.deletePatient(id);
			return responses::noContent();
		}
		catch (const std::exception& e) {
			return responses::internalError(e.what());
		}
	}
}