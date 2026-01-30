#include "add-anamnesis-ui.h"

#include <iostream>
#include <limits>
#include <optional>
#include <string>

#ifdef _WIN32
#include "console-utf8.h"
#endif

bool addAnamnesisUI(int patient_id, ApiClient& apiClient) {
#ifdef _WIN32 
    initWinConsoleUnicode();
    system("cls");
#else
    system("clear");
#endif
    std::cout << "Add anamnesis\n";
    std::cout << "Description (empty = cancel): ";

    std::string description;
#ifdef _WIN32
    description = readConsoleLineUtf8();
#else
    std::getline(std::cin, description);
#endif

    if (description.empty()) {
        return false;
    }

    std::cout << "Photo URL (empty = NULL): ";
    std::string photo_url;
#ifdef _WIN32
    photo_url = readConsoleLineUtf8();
#else
    std::getline(std::cin, photo_url);
#endif

    std::optional<std::string> photoOpt;
    if (!photo_url.empty()) {
        photoOpt = photo_url;
    }

    return apiClient.createAnamnesis(patient_id, description, photoOpt);
}