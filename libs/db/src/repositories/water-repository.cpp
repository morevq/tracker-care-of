#include "tracker_db/repositories/water-repository.h"

#include <userver/storages/postgres/io/optional.hpp>

namespace pg = userver::storages::postgres;

WaterRepository::WaterRepository(pg::ClusterPtr cluster)
    : cluster_(std::move(cluster)) {}

std::vector<Water> WaterRepository::getByUserUUID(const std::string& user_uuid) {
    auto result = cluster_->Execute(
        pg::ClusterHostType::kSlave,
        R"(SELECT
              w.id_water,
              w.last_water::text AS last_water,
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
        water.idWater          = row["id_water"].As<std::optional<int>>().value_or(0);
        water.idPatient        = row["id_patient"].As<int>();
        water.lastWater        = row["last_water"].As<std::optional<std::string>>().value_or("");
        water.frequency        = row["frequency"].As<std::optional<int>>().value_or(-1);
        water.frequencyMeasure = row["frequency_measure"].As<std::optional<std::string>>().value_or("");
        entries.push_back(std::move(water));
    }
    return entries;
}

std::vector<Water> WaterRepository::listByPatientID(int id_patient) {
    auto result = cluster_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT id_water, id_patient, last_water::text AS last_water "
        "FROM water WHERE id_patient = $1 "
        "ORDER BY last_water DESC",
        id_patient
    );

    std::vector<Water> entries;
    entries.reserve(result.Size());
    for (auto row : result) {
        Water water;
        water.idWater          = row["id_water"].As<int>();
        water.idPatient        = row["id_patient"].As<int>();
        water.lastWater        = row["last_water"].As<std::string>();
        water.frequency        = -1;
        water.frequencyMeasure = "";
        entries.push_back(std::move(water));
    }
    return entries;
}

std::optional<Water> WaterRepository::getByID(int id_water) {
    auto result = cluster_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT id_water, id_patient, last_water::text AS last_water "
        "FROM water WHERE id_water = $1",
        id_water
    );

    if (result.IsEmpty()) {
        return std::nullopt;
    }

    auto row = result[0];
    Water water;
    water.idWater          = row["id_water"].As<int>();
    water.idPatient        = row["id_patient"].As<int>();
    water.lastWater        = row["last_water"].As<std::string>();
    water.frequency        = -1;
    water.frequencyMeasure = "";
    return water;
}

std::optional<int> WaterRepository::addWater(int id_patient, const std::string& last_water) {
    auto check = cluster_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT birth_date::text AS birth_date FROM patient "
        "WHERE id_patient = $1 AND is_deleted = FALSE",
        id_patient
    );

    if (check.IsEmpty()) {
        return std::nullopt;
    }

    auto birth_date = check[0]["birth_date"].As<std::optional<std::string>>();
    if (birth_date.has_value() && !birth_date->empty() && last_water < *birth_date) {
        return std::nullopt;
    }

    auto result = cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "INSERT INTO water (id_patient, last_water) VALUES ($1, $2::timestamptz) "
        "RETURNING id_water",
        id_patient, last_water
    );
    if (result.IsEmpty()) {
        return std::nullopt;
    }
    return result[0]["id_water"].As<int>();
}

void WaterRepository::deleteByID(int id_water) {
    cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "DELETE FROM water WHERE id_water = $1",
        id_water
    );
}

std::optional<WaterFrequency> WaterRepository::getFrequency(int id_patient) {
    auto result = cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "SELECT id_patient, frequency, frequency_measure "
        "FROM water_frequency WHERE id_patient = $1",
        id_patient
    );

    if (result.IsEmpty()) {
        return std::nullopt;
    }

    auto row = result[0];
    auto frequency = row["frequency"].As<std::optional<int>>();
    auto measure   = row["frequency_measure"].As<std::optional<std::string>>();
    if (!frequency || !measure) {
        return std::nullopt;
    }

    WaterFrequency wf;
    wf.idPatient = row["id_patient"].As<int>();
    wf.frequency = *frequency;
    wf.measure   = *measure;
    return wf;
}

void WaterRepository::upsertFrequency(int id_patient, int frequency,
                                      const std::string& frequency_measure) {
    cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "INSERT INTO water_frequency (id_patient, frequency, frequency_measure) "
        "VALUES ($1, $2, $3) "
        "ON CONFLICT (id_patient) DO UPDATE SET "
        "  frequency = EXCLUDED.frequency, "
        "  frequency_measure = EXCLUDED.frequency_measure",
        id_patient, frequency, frequency_measure
    );
}

void WaterRepository::deleteFrequency(int id_patient) {
    cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "DELETE FROM water_frequency WHERE id_patient = $1",
        id_patient
    );
}
