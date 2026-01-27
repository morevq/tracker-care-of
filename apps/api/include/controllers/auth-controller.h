#pragma once

#include <crow.h>
#include <tracker/usecases/auth-service.h>

#include "../dto/auth-dto.h"

namespace tracker_api {

    class AuthController {
    private:
        tracker::AuthService& authService;

        crow::response registerUser(const crow::request& req);
        crow::response loginUser(const crow::request& req);

    public:
        AuthController(tracker::AuthService& authService);

        void registerRoutes(crow::SimpleApp& app);
    };

}