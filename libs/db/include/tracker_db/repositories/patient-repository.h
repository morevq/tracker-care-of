#pragma once

#include <string>
#include <vector>
#include <libpq-fe.h>
#include <memory>
#include <tracker_db/db-utils.h>

#include "tracker/models/patient.h"

class PatientRepository {
private:
	db_utils::PGconnPtr connection;

public:
	PatientRepository(db_utils::PGconnPtr connnection);

	std::vector<Patient> getByUserUUID(const std::string& user_uuid);
	std::optional<Patient> getByID(int id_patient);
	void createPatient(const std::string& user_uuid, const std::string& name, std::optional<std::string> birth_date);
	void updatePatient(int id_patient, const std::optional<std::string> name, std::optional<std::string> birth_date);
	void deletePatient(int id_patient);
};