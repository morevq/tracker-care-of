#include "controllers/anamnesis-controller.h"
#include "middleware/auth-middleware.h"
#include <tracker_common/http-responses.h>
#include <nlohmann/json.hpp>

#ifdef DELETE
#undef DELETE
#endif

using json = nlohmann::json;

namespace tracker_api {

    AnamnesisController::AnamnesisController(AnamnesisRepository& anamnesisRepo, PatientRepository& patientRepo)
        : anamnesisRepo(anamnesisRepo), patientRepo(patientRepo) {}

    crow::Blueprint AnamnesisController::getBlueprint() {
        crow::Blueprint bp("api/anamnesis");
        
        CROW_BP_ROUTE(bp, "/<int>")
            .methods(crow::HTTPMethod::GET)
            ([this](const crow::request& req, int patientId) {
                return this->getAnamnesisData(req, patientId);
        });

        CROW_BP_ROUTE(bp, "/")
            .methods(crow::HTTPMethod::POST)
            ([this](const crow::request& req) {
                return this->createAnamnesis(req);
        });

        CROW_BP_ROUTE(bp, "/<int>")
            .methods(crow::HTTPMethod::DELETE)
            ([this](const crow::request& req, int id) {
                return this->deleteAnamnesis(req, id);
        });

        CROW_BP_ROUTE(bp, "/<int>")
            .methods("PATCH"_method)
            ([this](const crow::request& req, int id) {
                return updateAnamnesis(req, id);
        });

        return bp;
    }

    crow::response AnamnesisController::getAnamnesisData(const crow::request& req, int patientId) {
        try {
            auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
            if (!userUuid) {
                return responses::unauthorized();
            }

            auto patient = patientRepo.getByID(patientId);
            if (!patient) {
                return responses::notFound("Patient not found");
            }

            if (patient->user_uuid != *userUuid) {
                return responses::forbidden();
            }

            auto anamnesisRecords = anamnesisRepo.getByPatientId(patientId);
        
            std::vector<AnamnesisResponse> response;
            for (const auto& anamnesis : anamnesisRecords) {
                response.push_back({
                    anamnesis.id,
                    anamnesis.description,
                    anamnesis.photo_url,
                    anamnesis.created_at
                });
            }

            return crow::response(crow::status::OK, json(response).dump());
        }
        catch (const std::exception& e) {
            return responses::internalError(e.what());
        }
    }

    crow::response AnamnesisController::createAnamnesis(const crow::request& req) {
        try {
            auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
            if (!userUuid) {
                return responses::unauthorized();
            }

            auto requestData = json::parse(req.body);
            int patientId = requestData["patient_id"];
            std::string description = requestData["description"];
            std::optional<std::string> photoUrl = requestData["photo_url"].is_null() 
                ? std::nullopt 
                : std::optional<std::string>(requestData["photo_url"]);

            auto patient = patientRepo.getByID(patientId);
            if (!patient) {
                return responses::notFound("Patient not found");
            }

            if (patient->user_uuid != *userUuid) {
                return responses::forbidden();
            }

            anamnesisRepo.createAnamnesis(patientId, description, photoUrl);

            return responses::created("Anamnesis created successfully");
        }
        catch (const std::exception& e) {
            return responses::badRequest(e.what());
        }
    }

    crow::response AnamnesisController::updateAnamnesis(const crow::request& req, int id) {
        try {
            auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
            if (!userUuid) {
                return responses::unauthorized();
            }

            auto anamnesis = anamnesisRepo.getByID(id);
            if (!anamnesis.has_value()) {
                return responses::notFound("Anamnesis not found");
            }

            auto patient = patientRepo.getByID(anamnesis->id_patient);
            if (!patient || patient->user_uuid != *userUuid) {
                return responses::forbidden();
            }

            auto body = crow::json::load(req.body);
            if (!body) {
                return responses::badRequest(responses::messages::INVALID_JSON);
            }

            std::optional<std::string> description;
            std::optional<std::string> date;
            std::optional<std::string> photo_url;

            if (body.has("description")) {
                description = body["description"].s();
            }

            if (body.has("date")) {
                date = body["date"].s();
            }

            if (body.has("photo_url")) {
                photo_url = body["photo_url"].s();
            }

            if (!description.has_value() && !date.has_value() && !photo_url.has_value()) {
                return responses::badRequest("At least one field must be provided");
            }

            anamnesisRepo.updateAnamnesis(id, description, date, photo_url);

            auto updated = anamnesisRepo.getByID(id);
            crow::json::wvalue response;
            response["id"] = updated->id;
            response["patient_id"] = updated->id_patient;
            response["description"] = updated->description;
            if (updated->photo_url.has_value()) {
                response["photo_url"] = *updated->photo_url;
            }
            response["created_at"] = updated->created_at;

            return crow::response(crow::status::OK, response);
        }
        catch (const std::exception& e) {
            return responses::internalError(e.what());
        }
    }

    crow::response AnamnesisController::deleteAnamnesis(const crow::request& req, int id) {
        try {
            auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
            if (!userUuid) {
                return responses::unauthorized();
            }

            anamnesisRepo.deleteAnamnesis(id);
            return crow::response(204);
        }
        catch (const std::exception& e) {
            return responses::internalError(e.what());
        }
    }

}