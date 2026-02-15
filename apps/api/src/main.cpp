#include <crow.h>
#include <libpq-fe.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <fstream>

#include <tracker_db/usecases/auth-service.h>
#include <tracker_db/repositories/patient-repository.h>
#include <tracker_db/repositories/water-repository.h>
#include <tracker_db/repositories/anamnesis-repository.h>
#include <tracker_db/db-utils.h>
#include <tracker_common/env-parser.h>

#include <tracker_session/redis-session-store.h>

#include "controllers/auth-controller.h"
#include "controllers/patient-controller.h"
#include "controllers/water-controller.h"
#include "controllers/anamnesis-controller.h"
#include "middleware/auth-middleware.h"

static std::string buildRedisUri(const std::unordered_map<std::string, std::string>& env) {
    auto it = env.find("REDIS_URI");
    if (it != env.end() && !it->second.empty()) return it->second;

    std::string host = get_env_var(env, "REDIS_HOST", "127.0.0.1");
    std::string port = get_env_var(env, "REDIS_PORT", "6379");
    return "tcp://" + host + ":" + port;
}

int main() {
    try {
        auto env = load_env(".env");

        std::ostringstream conninfo;
        conninfo << "host=" << get_env_var(env, "DB_HOST")
            << " port=" << get_env_var(env, "DB_PORT", "5432")
            << " dbname=" << get_env_var(env, "DB_NAME")
            << " user=" << get_env_var(env, "DB_USER")
            << " password=" << get_env_var(env, "DB_PASSWORD");

        auto conn = db_utils::make_pgconn(conninfo.str().c_str());

        if (PQstatus(conn.get()) != CONNECTION_OK) {
            std::cerr << "Connection to database failed: " << PQerrorMessage(conn.get()) << std::endl;
            return 1;
        }
        std::cout << "Connected to database successfully!" << std::endl;

        std::string redisUri = buildRedisUri(env);
        auto sessionStore = std::make_shared<tracker_session::RedisSessionStore>(redisUri);
        tracker_api::AuthMiddleware::init(sessionStore);
        std::cout << "Redis session store: " << redisUri << std::endl;

        AuthService authService(conn);
        PatientRepository patientRepo(conn);
        WaterRepository waterRepo(conn);
        AnamnesisRepository anamnesisRepo(conn);

        tracker_api::AuthController authController(authService, sessionStore);
        tracker_api::PatientController patientController(patientRepo);
        tracker_api::WaterController waterController(waterRepo, patientRepo);
        tracker_api::AnamnesisController anamnesisController(anamnesisRepo, patientRepo);

        crow::SimpleApp app;

        auto authBp = authController.getBlueprint();
        app.register_blueprint(authBp);

        auto patientBp = patientController.getBlueprint();
        app.register_blueprint(patientBp);

        auto waterBp = waterController.getBlueprint();
        app.register_blueprint(waterBp);

        auto anamnesisBp = anamnesisController.getBlueprint();
        app.register_blueprint(anamnesisBp);

        CROW_ROUTE(app, "/swagger.json")
            ([]() {
            std::ifstream file("apps/api/swagger.json");
            if (!file.is_open()) {
                return crow::response(crow::status::INTERNAL_SERVER_ERROR, "Swagger spec not found");
            }
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            crow::response res(content);
            res.add_header("Content-Type", "application/json");
            return res;
                });

        CROW_ROUTE(app, "/swagger")
            ([]() {
            return R"(<!DOCTYPE html>
                <html>
                <head><link rel="stylesheet" href="https://unpkg.com/swagger-ui-dist/swagger-ui.css"/></head>
                <body>
                <div id="swagger-ui"></div>
                <script src="https://unpkg.com/swagger-ui-dist/swagger-ui-bundle.js"></script>
                <script>SwaggerUIBundle({url:'/swagger.json',dom_id:'#swagger-ui'});</script>
                </body>
                </html>)";
                });

        int port = env.count("API_PORT") ? std::stoi(env.at("API_PORT")) : 8080;

        std::cout << "Starting server on port " << port << "..." << std::endl;
        std::cout << "Swagger UI: http://localhost:" << port << "/swagger" << std::endl;

        app.port(static_cast<std::uint16_t>(port)).multithreaded().run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
