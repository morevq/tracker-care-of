#include "add-patient-ui.h"

#include <iostream>
#include <limits>
#include <optional>

#ifdef _WIN32
#include "console-utf8.h"
#endif

bool addPatientUI(ApiClient& apiClient) {
#ifdef _WIN32
    initWinConsoleUnicode();
    system("cls");
#else
    system("clear");
#endif

    std::cout << "Add patient\n";
    std::cout << "Name (empty = cancel): ";

    std::string name;
#ifdef _WIN32
    name = readConsoleLineUtf8();
#else
    std::getline(std::cin, name);
#endif

    if (name.empty()) {
        return false;
    }

    std::cout << "Birth date YYYY-MM-DD (empty = NULL): ";
    std::string birth;
#ifdef _WIN32
    birth = readConsoleLineUtf8();
#else
    std::getline(std::cin, birth);
#endif

    std::optional<std::string> birthOpt;
    if (!birth.empty()) {
        birthOpt = birth;
    }

    return apiClient.createPatient(name, birthOpt);
}