#include "controllers/swagger-controller.h"
#include <fstream>
#include <sstream>

namespace tracker_api {
    SwaggerController::SwaggerController(crow::SimpleApp& app) : bp_("") {
        setupRoutes();
        app.register_blueprint(bp_);
    }
    void SwaggerController::setupRoutes() {

        CROW_BP_ROUTE(bp_, "/swagger.json")
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

        CROW_BP_ROUTE(bp_, "/swagger")
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
    }

}
