#pragma once

#include <string>
#include <optional>

struct Patient {
    int id_patient;
    std::string user_uuid;
    std::string name;
    std::optional<std::string> birth_date;
	std::string created_at;

    std::string getAge() const;
};