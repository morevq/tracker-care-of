#pragma once

#include <crow.h>
#include <memory>
#include <tracker_db/db-utils.h>
#include <tracker_db/usecases/auth-service.h>
#include <tracker_db/repositories/patient-repository.h>
#include <tracker_db/repositories/water-repository.h>
#include <tracker_db/repositories/anamnesis-repository.h>
#include <tracker_session/redis-session-store.h>

#include "config.h"
#include "controllers/auth-controller.h"
#include "controllers/patient-controller.h"
#include "controllers/water-controller.h"
#include "controllers/anamnesis-controller.h"
#include "controllers/swagger-controller.h"

namespace tracker_api {

    class ApiServer {
    public:
        explicit ApiServer(const Config& config);
        void run();

    private:
        const Config& config_;
        crow::SimpleApp app_;
        
        db_utils::PGconnPtr conn_;
        std::shared_ptr<tracker_session::RedisSessionStore> sessionStore_;

        std::unique_ptr<AuthService> authService_;
        std::unique_ptr<PatientRepository> patientRepo_;
        std::unique_ptr<WaterRepository> waterRepo_;
        std::unique_ptr<AnamnesisRepository> anamnesisRepo_;

        std::unique_ptr<AuthController> authController_;
        std::unique_ptr<PatientController> patientController_;
        std::unique_ptr<WaterController> waterController_;
        std::unique_ptr<AnamnesisController> anamnesisController_;
        std::unique_ptr<SwaggerController> swaggerController_;
    };

}
