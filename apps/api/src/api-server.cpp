#include "api-server.h"
#include "middleware/auth-middleware.h"
#include <iostream>

namespace tracker_api {

    ApiServer::ApiServer(const Config& config) : config_(config) {
        conn_ = db_utils::make_pgconn(config_.getDbConnInfo().c_str());
        if (PQstatus(conn_.get()) != CONNECTION_OK) {
            throw std::runtime_error("Connection to database failed: " + std::string(PQerrorMessage(conn_.get())));
        }
        std::cout << "Connected to database successfully!" << std::endl;

        std::string redisUri = config_.getRedisUri();
        sessionStore_ = std::make_shared<tracker_session::RedisSessionStore>(redisUri);
        AuthMiddleware::init(sessionStore_);
        std::cout << "Redis session store: " << redisUri << std::endl;

        authService_ = std::make_unique<AuthService>(conn_);
        patientRepo_ = std::make_unique<PatientRepository>(conn_);
        waterRepo_ = std::make_unique<WaterRepository>(conn_);
        anamnesisRepo_ = std::make_unique<AnamnesisRepository>(conn_);

        authController_ = std::make_unique<AuthController>(*authService_, sessionStore_, app_);
        patientController_ = std::make_unique<PatientController>(*patientRepo_, app_);
        waterController_ = std::make_unique<WaterController>(*waterRepo_, *patientRepo_, app_);
        anamnesisController_ = std::make_unique<AnamnesisController>(*anamnesisRepo_, *patientRepo_, app_);
        swaggerController_ = std::make_unique<SwaggerController>(app_);
    }

    void ApiServer::run() {
        int port = config_.getApiPort();
        std::cout << "Starting server on port " << port << "..." << std::endl;
        std::cout << "Swagger UI: http://localhost:" << port << "/swagger" << std::endl;

        app_.port(static_cast<std::uint16_t>(port)).multithreaded().run();
    }

}
