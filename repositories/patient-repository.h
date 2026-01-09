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
};