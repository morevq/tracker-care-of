#pragma once

#include <crow.h>

namespace tracker_api {

    class SwaggerController {
    private:
        crow::SimpleApp& app_;
        void setupRoutes();
    public:
        SwaggerController(crow::SimpleApp& app);
    };

}
