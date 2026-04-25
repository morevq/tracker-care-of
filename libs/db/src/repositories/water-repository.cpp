#include "tracker_db/repositories/water-repository.h"

#include <userver/storages/postgres/io/optional.hpp>

namespace pg = userver::storages::postgres;

WaterRepository::WaterRepository(pg::ClusterPtr cluster)
    : cluster_(std::move(cluster)) {}

std::vector<Water> WaterRepository::getByUserUUID(const std::string& user_uuid) {
    auto result = cluster_->Execute(
        pg::ClusterHostType::kSlave,
        R"(SELECT
              w.last_water,
              wf.frequency,
              wf.frequency_measure,
              p.id_patient
           FROM patient p
           LEFT JOIN water w ON w.id_patient = p.id_patient
           LEFT JOIN water_frequency wf ON wf.id_patient = p.id_patient
           WHERE p.user_uuid = $1::uuid
           ORDER BY w.last_water DESC NULLS LAST)",
        user_uuid
    );

    std::vector<Water> entries;
    entries.reserve(result.Size());
    for (auto row : result) {
        Water water;
        water.idPatient        = row["id_patient"].As<int>();
        water.lastWater        = row["last_water"].As<std::optional<std::string>>().value_or("");
        water.frequency        = row["frequency"].As<std::optional<int>>().value_or(-1);
        water.frequencyMeasure = row["frequency_measure"].As<std::optional<std::string>>().value_or("");
        entries.push_back(std::move(water));
    }
    return entries;
}

std::optional<Water> WaterRepository::getByPatientID(int id_patient) {
    auto result = cluster_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT w.id_patient, w.last_water, wf.frequency, wf.frequency_measure "
        "FROM water AS w "
        "LEFT JOIN water_frequency AS wf ON w.id_patient = wf.id_patient "
        "WHERE w.id_patient = $1 "
        "ORDER BY w.last_water DESC "
        "LIMIT 1",
        id_patient
    );

    if (result.IsEmpty()) {
        return std::nullopt;
    }

    auto row = result[0];
    Water water;
    water.idPatient        = row["id_patient"].As<int>();
    water.lastWater        = row["last_water"].As<std::string>();
    water.frequency        = row["frequency"].As<std::optional<int>>().value_or(-1);
    water.frequencyMeasure = row["frequency_measure"].As<std::optional<std::string>>().value_or("");
    return water;
}

bool WaterRepository::addWater(int id_patient, const std::string& last_water) {
    auto check = cluster_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT birth_date FROM patient "
        "WHERE id_patient = $1 AND is_deleted = FALSE",
        id_patient
    );

    if (check.IsEmpty()) {
        return false;
    }

    auto birth_date = check[0]["birth_date"].As<std::optional<std::string>>();
    if (birth_date.has_value() && !birth_date->empty() && last_water < *birth_date) {
        return false;
    }

    cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "INSERT INTO water (id_patient, last_water) VALUES ($1, $2)",
        id_patient, last_water
    );
    return true;
}

void WaterRepository::deleteWater(int id_patient) {
    cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "DELETE FROM water WHERE id_patient = $1",
        id_patient
    );
}
