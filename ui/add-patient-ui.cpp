#include "add-patient-ui.h"

#include <iostream>
#include <limits>
#include <optional>

#include "../repositories/patient-repository.h"

#ifdef _WIN32
#include "console-utf8.h"
#endif

bool addPatientUI(const std::string& user_uuid, PGconn* connection) {
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

    PatientRepository repo(connection);
    repo.createPatient(user_uuid, name, birthOpt);

    return true;
}