#include "add-anamnesis-ui.h"

#include <iostream>
#include <limits>
#include <optional>
#include <string>

#include "../repositories/anamnesis-repository.h"

bool addAnamnesisUI(int patient_id, PGconn* connection) {
#ifdef _WIN32 
    system("cls");
#else
    system("clear");
#endif
    std::cout << "Add anamnesis\n";
    std::cout << "Description (empty = cancel): ";

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::string description;
    std::getline(std::cin, description);

    if (description.empty()) {
        return false;
    }

    std::cout << "Photo URL (empty = NULL): ";
    std::string photo_url;
    std::getline(std::cin, photo_url);

    std::optional<std::string> photoOpt;
    if (!photo_url.empty()) {
        photoOpt = photo_url;
    }

    AnamnesisRepository repo(connection);
    repo.createAnamnesis(patient_id, description, photoOpt);

    return true;
}