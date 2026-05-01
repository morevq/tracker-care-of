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
    std::optional<Water> getByPatientID(int id_patient);
    bool addWater(int id_patient, const std::string& last_water);
    void deleteWater(int id_patient);

private:
    userver::storages::postgres::ClusterPtr cluster_;
};
