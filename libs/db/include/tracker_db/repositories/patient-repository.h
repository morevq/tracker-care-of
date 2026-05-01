#pragma once

#include <optional>
#include <string>
#include <vector>

#include <userver/storages/postgres/cluster.hpp>

#include "tracker/models/patient.h"

class PatientRepository {
public:
    explicit PatientRepository(userver::storages::postgres::ClusterPtr cluster);

    std::vector<Patient> getByUserUUID(const std::string& user_uuid);
    std::optional<Patient> getByID(int id_patient);
    void createPatient(const std::string& user_uuid,
                       const std::string& name,
                       std::optional<std::string> birth_date);
    void updatePatient(int id_patient,
                       std::optional<std::string> name,
                       std::optional<std::string> birth_date);
    void deletePatient(int id_patient);

private:
    userver::storages::postgres::ClusterPtr cluster_;
};
