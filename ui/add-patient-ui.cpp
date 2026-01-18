#include "add-patient-ui.h"

#include <iostream>
#include <limits>
#include <optional>

#include "../repositories/patient-repository.h"

bool addPatientUI(const std::string& user_uuid, PGconn* connection) {
#ifdef _WIN32 
    system("cls");
#else
    system("clear");
#endif
    std::cout << "Add patient\n";
    std::cout << "Name (empty = cancel): ";

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::string name;
    std::getline(std::cin, name);

    if (name.empty()) {
        return false;
    }

    std::cout << "Birth date YYYY-MM-DD (empty = NULL): ";
    std::string birth;
    std::getline(std::cin, birth);

    std::optional<std::string> birthOpt;
    if (!birth.empty()) {
        birthOpt = birth;
    }

    PatientRepository repo(connection);
    repo.createPatient(user_uuid, name, birthOpt);

    return true;
}