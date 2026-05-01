#include "tracker_db/repositories/anamnesis-repository.h"

#include <userver/storages/postgres/io/optional.hpp>

namespace pg = userver::storages::postgres;

AnamnesisRepository::AnamnesisRepository(pg::ClusterPtr cluster)
    : cluster_(std::move(cluster)) {}

std::vector<Anamnesis> AnamnesisRepository::getByPatientId(int id_patient) {
    auto result = cluster_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT id_anamnesis, description, date::text, photo_url "
        "FROM anamnesis WHERE id_patient = $1 AND is_deleted = FALSE",
        id_patient
    );

    std::vector<Anamnesis> entries;
    entries.reserve(result.Size());
    for (auto row : result) {
        Anamnesis a;
        a.id          = row["id_anamnesis"].As<int>();
        a.id_patient  = id_patient;
        a.description = row["description"].As<std::string>();
        a.created_at  = row["date"].As<std::string>();
        a.photo_url   = row["photo_url"].As<std::optional<std::string>>();
        entries.push_back(std::move(a));
    }
    return entries;
}

std::optional<Anamnesis> AnamnesisRepository::getByID(int id_anamnesis) {
    auto result = cluster_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT id_anamnesis, id_patient, description, photo_url, date::text "
        "FROM anamnesis WHERE id_anamnesis = $1 AND is_deleted = FALSE",
        id_anamnesis
    );

    if (result.IsEmpty()) {
        return std::nullopt;
    }

    auto row = result[0];
    Anamnesis a;
    a.id          = row["id_anamnesis"].As<int>();
    a.id_patient  = row["id_patient"].As<int>();
    a.description = row["description"].As<std::string>();
    a.photo_url   = row["photo_url"].As<std::optional<std::string>>();
    a.created_at  = row["date"].As<std::string>();
    return a;
}

void AnamnesisRepository::createAnamnesis(int id_patient,
                                          std::string description,
                                          std::optional<std::string> photo_url) {
    cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "INSERT INTO anamnesis (id_patient, description, photo_url) "
        "VALUES ($1, $2, $3)",
        id_patient, description, photo_url
    );
}

void AnamnesisRepository::updateAnamnesis(int id_anamnesis,
                                          std::optional<std::string> description,
                                          std::optional<std::string> date,
                                          std::optional<std::string> photo_url) {
    if (!description.has_value() && !date.has_value() && !photo_url.has_value()) {
        return;
    }

    cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "UPDATE anamnesis SET "
        "  description = COALESCE($2, description), "
        "  date        = COALESCE($3::timestamptz, date), "
        "  photo_url   = COALESCE($4, photo_url) "
        "WHERE id_anamnesis = $1 AND is_deleted = FALSE",
        id_anamnesis, description, date, photo_url
    );
}

void AnamnesisRepository::deleteAnamnesis(int id_anamnesis) {
    cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "UPDATE anamnesis SET is_deleted = TRUE WHERE id_anamnesis = $1",
        id_anamnesis
    );
}
