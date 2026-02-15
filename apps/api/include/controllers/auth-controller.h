#pragma once

#include <crow.h>
#include <memory>

#include <tracker_db/usecases/auth-service.h>
#include <tracker_session/session-store.h>

#include "../dto/auth-dto.h"

namespace tracker_api {

    class AuthController {
    private:
        AuthService& authService;
        std::shared_ptr<tracker_session::SessionStore> sessionStore_;

        crow::response registerUser(const crow::request& req);
        crow::response loginUser(const crow::request& req);
        crow::response logoutUser(const crow::request& req);
        crow::response updateUser(const crow::request& req);
        crow::response deleteUser(const crow::request& req);

    public:
        AuthController(AuthService& authService,
            std::shared_ptr<tracker_session::SessionStore> sessionStore);

        crow::Blueprint getBlueprint();
    };

} 