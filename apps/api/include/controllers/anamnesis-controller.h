#pragma once
#include <crow.h>
#include <tracker_db/repositories/anamnesis-repository.h>
#include <tracker_db/repositories/patient-repository.h>
#include "../dto/anamnesis-dto.h"

namespace tracker_api {

    class AnamnesisController {
    private:
        AnamnesisRepository& anamnesisRepo;
        PatientRepository& patientRepo;

        crow::response getAnamnesisData(const crow::request& req, int patientId);
        crow::response createAnamnesis(const crow::request& req);
        crow::response deleteAnamnesis(const crow::request& req, int id);

    public:
        AnamnesisController(AnamnesisRepository& anamnesisRepo, PatientRepository& patientRepo);

        void registerRoutes(crow::SimpleApp& app);
    };

}