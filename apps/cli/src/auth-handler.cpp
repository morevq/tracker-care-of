#include "auth-handler.h"
#include "console-utils.h"
#include <iostream>
#include <limits>

bool AuthHandler::authenticate(ApiClient& apiClient) {
    std::cout << "1) Register\n2) Login\n> ";
    int mode = 0;
    if (!(std::cin >> mode)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cerr << "Invalid input. Please enter 1 or 2.\n";
        return false;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (mode != 1 && mode != 2) {
        std::cerr << "Invalid mode. Please choose 1 or 2.\n";
        return false;
    }

    std::string email;
    std::string password;

    std::cout << "Email: ";
    std::cin >> email;
    std::cout << "Password: ";
    password = readPassword();

    return performAuth(apiClient, mode, email, password);
}

bool AuthHandler::performAuth(ApiClient& apiClient, int mode, const std::string& email, const std::string& password) {
    bool authSuccess = false;
    if (mode == 1) {
        authSuccess = apiClient.registerUser(email, password);
    } else {
        authSuccess = apiClient.loginUser(email, password);
    }

    if (!authSuccess) {
        std::cerr << "Auth failed.\n";
    }
    return authSuccess;
}
