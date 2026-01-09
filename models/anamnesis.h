#pragma once

#include <string>
#include <optional>

struct Anamnesis {
    int id;
    int id_patient;
    std::string description;
    std::optional<std::string> photo_url;
    std::string date;
};