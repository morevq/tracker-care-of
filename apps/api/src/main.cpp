#include <crow.h>
#include <libpq-fe.h>
#include <iostream>
#include <memory>
#include <sstream>

#include <tracker/usecases/auth-service.h>
#include <tracker_db/repositories/patient-repository.h>
#include <tracker_db/repositories/water-repository.h>
#include <tracker_db/repositories/anamnesis-repository.h>
#include <tracker_common/env-parser.h>

#include "controllers/auth-controller.h"
#include "controllers/patient-controller.h"
#include "controllers/water-controller.h"
#include "controllers/anamnesis-controller.h"

int main() {
    try {
        auto env = load_env(".env");

        std::ostringstream conninfo;
        conninfo << "host=" << env["DB_HOST"]
                 << " port=" << (env.count("DB_PORT") ? env["DB_PORT"] : "5432")
                 << " dbname=" << env["DB_NAME"]
                 << " user=" << env["DB_USER"]
                 << " password=" << env["DB_PASSWORD"];

        PGconn* conn = PQconnectdb(conninfo.str().c_str());

        if (PQstatus(conn) != CONNECTION_OK) {
            std::cerr << "Connection to database failed: " << PQerrorMessage(conn) << std::endl;
            PQfinish(conn);
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

        int port = env.count("API_PORT") ? std::stoi(env["API_PORT"]) : 8080;
        
        std::cout << "Starting server on port " << port << "..." << std::endl;
        app.port(static_cast<std::uint16_t>(port)).multithreaded().run();

        PQfinish(conn);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
