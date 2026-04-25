#pragma once

#include <optional>
#include <string>
#include <vector>

#include <userver/storages/postgres/cluster.hpp>

#include "tracker/models/anamnesis.h"

class AnamnesisRepository {
public:
    explicit AnamnesisRepository(userver::storages::postgres::ClusterPtr cluster);

    std::vector<Anamnesis> getByPatientId(int id_patient);
    std::optional<Anamnesis> getByID(int id_anamnesis);
    void createAnamnesis(int id_patient,
                         std::string description,
                         std::optional<std::string> photo_url);
    void updateAnamnesis(int id_anamnesis,
                         std::optional<std::string> description,
                         std::optional<std::string> date,
                         std::optional<std::string> photo_url);
    void deleteAnamnesis(int id_anamnesis);

private:
    userver::storages::postgres::ClusterPtr cluster_;
};
