#include "controllers/anamnesis-controller.h"
#include "middleware/auth-middleware.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tracker_api {

    AnamnesisController::AnamnesisController(AnamnesisRepository& anamnesisRepo, PatientRepository& patientRepo)
        : anamnesisRepo(anamnesisRepo), patientRepo(patientRepo) {}

    void AnamnesisController::registerRoutes(crow::SimpleApp& app) {
        CROW_ROUTE(app, "/api/anamnesis/<int>")
            .methods(crow::HTTPMethod::GET)
            ([this](const crow::request& req, int patientId) {
                return this->getAnamnesisData(req, patientId);
            });
    }

    crow::response AnamnesisController::getAnamnesisData(const crow::request& req, int patientId) {
        try {
            auto userUuid = AuthMiddleware::getUserUuidFromCookie(req);
            if (!userUuid) {
                return crow::response(401, "Unauthorized");
            }

            auto patient = patientRepo.getByID(patientId);
            if (!patient) {
                return crow::response(404, "Patient not found");
            }

            if (patient->user_uuid != *userUuid) {
                return crow::response(403, "Forbidden: Access denied");
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

            return crow::response(200, json(response).dump());
        }
        catch (const std::exception& e) {
            return crow::response(500, "Internal server error: " + std::string(e.what()));
        }
    }

}