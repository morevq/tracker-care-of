#pragma once

#include <optional>
#include <string>
#include <vector>

#include <userver/storages/postgres/cluster.hpp>

#include "tracker/models/water.h"

class WaterRepository {
public:
    explicit WaterRepository(userver::storages::postgres::ClusterPtr cluster);

    std::vector<Water> getByUserUUID(const std::string& user_uuid);
    std::vector<Water> listByPatientID(int id_patient);
    std::optional<Water> getByID(int id_water);
    std::optional<int> addWater(int id_patient, const std::string& last_water);
    void deleteByID(int id_water);

    std::optional<WaterFrequency> getFrequency(int id_patient);
    void upsertFrequency(int id_patient, int frequency,
                         const std::string& frequency_measure);
    void deleteFrequency(int id_patient);

private:
    userver::storages::postgres::ClusterPtr cluster_;
};
