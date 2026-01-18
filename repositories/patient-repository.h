#pragma once

#include <string>
#include <vector>
#include <libpq-fe.h>

#include "../models/patient.h"

class PatientRepository {
private:
	PGconn* connection;

public:
	PatientRepository(PGconn* connnection);

	std::vector<Patient> getByUserUUID(const std::string& user_uuid);
	void createPatient(const std::string& user_uuid, const std::string& name, std::optional<std::string> birth_date);
	void deletePatient(int id_patient);
};