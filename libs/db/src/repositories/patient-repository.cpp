#include "tracker_db/repositories/patient-repository.h"

#include <userver/storages/postgres/io/optional.hpp>

namespace pg = userver::storages::postgres;

PatientRepository::PatientRepository(pg::ClusterPtr cluster)
    : cluster_(std::move(cluster)) {}

std::vector<Patient> PatientRepository::getByUserUUID(const std::string& user_uuid) {
    auto result = cluster_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT id_patient, user_uuid::text, name, birth_date::text "
        "FROM patient WHERE user_uuid = $1::uuid AND is_deleted = FALSE",
        user_uuid
    );

    std::vector<Patient> patients;
    patients.reserve(result.Size());
    for (auto row : result) {
        Patient p;
        p.id_patient = row["id_patient"].As<int>();
        p.user_uuid  = row["user_uuid"].As<std::string>();
        p.name       = row["name"].As<std::string>();
        p.birth_date = row["birth_date"].As<std::optional<std::string>>();
        patients.push_back(std::move(p));
    }
    return patients;
}

std::optional<Patient> PatientRepository::getByID(int id_patient) {
    auto result = cluster_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT id_patient, user_uuid::text, name, birth_date::text "
        "FROM patient WHERE id_patient = $1 AND is_deleted = FALSE",
        id_patient
    );

    if (result.IsEmpty()) {
        return std::nullopt;
    }

    auto row = result[0];
    Patient p;
    p.id_patient = row["id_patient"].As<int>();
    p.user_uuid  = row["user_uuid"].As<std::string>();
    p.name       = row["name"].As<std::string>();
    p.birth_date = row["birth_date"].As<std::optional<std::string>>();
    return p;
}

void PatientRepository::createPatient(const std::string& user_uuid,
                                      const std::string& name,
                                      std::optional<std::string> birth_date) {
    cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "INSERT INTO patient (user_uuid, name, birth_date) VALUES ($1::uuid, $2, $3::date)",
        user_uuid, name, birth_date
    );
}

void PatientRepository::updatePatient(int id_patient,
                                      std::optional<std::string> name,
                                      std::optional<std::string> birth_date) {
    if (!name.has_value() && !birth_date.has_value()) {
        return;
    }

    cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "UPDATE patient SET "
        "  name       = COALESCE($2, name), "
        "  birth_date = COALESCE($3::date, birth_date) "
        "WHERE id_patient = $1 AND is_deleted = FALSE",
        id_patient, name, birth_date
    );
}

void PatientRepository::deletePatient(int id_patient) {
    cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "UPDATE patient SET is_deleted = TRUE WHERE id_patient = $1",
        id_patient
    );
}
