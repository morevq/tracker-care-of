#pragma once

#include <crow.h>

namespace tracker_api {

    class SwaggerController {
    private:
        crow::Blueprint bp_;
        void setupRoutes();
    public:
        SwaggerController(crow::SimpleApp& app);
    };

}
