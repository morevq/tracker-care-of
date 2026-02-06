#include <crow.h>
#include <libpq-fe.h>
#include <iostream>
#include <memory>
#include <sstream>

#include <tracker/usecases/auth-service.h>
#include <tracker_db/repositories/patient-repository.h>
#include <tracker_db/repositories/water-repository.h>
#include <tracker_db/repositories/anamnesis-repository.h>
#include <tracker_db/db-utils.h>
#include <tracker_common/env-parser.h>

#include "controllers/auth-controller.h"
#include "controllers/patient-controller.h"
#include "controllers/water-controller.h"
#include "controllers/anamnesis-controller.h"

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

        AuthService authService(conn);
        PatientRepository patientRepo(conn);
        WaterRepository waterRepo(conn);
        AnamnesisRepository anamnesisRepo(conn);

        tracker_api::AuthController authController(authService);
        tracker_api::PatientController patientController(patientRepo);
        tracker_api::WaterController waterController(waterRepo);
        tracker_api::AnamnesisController anamnesisController(anamnesisRepo, patientRepo);

        crow::SimpleApp app;

        authController.registerRoutes(app);
        patientController.registerRoutes(app);
        waterController.registerRoutes(app);
        anamnesisController.registerRoutes(app);

        CROW_ROUTE(app, "/swagger.json")
            ([]() {
                std::ifstream file("apps/api/swagger.json");
                if (!file.is_open()) {
                    return crow::response(500, "Swagger spec not found");
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

        int port = env.count("API_PORT") ? std::stoi(env["API_PORT"]) : 8080;
        
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
