#pragma once

#include "api-client.h"
#include <string>

class AuthHandler {
public:
    static bool authenticate(ApiClient& apiClient);

private:
    static bool performAuth(ApiClient& apiClient, int mode, const std::string& email, const std::string& password);
};
