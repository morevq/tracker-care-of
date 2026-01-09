#pragma once

#include <string>
#include <vector>
#include <libpq-fe.h>

#include "../models/anamnesis.h"

class AnamnesisRepository {
private:
	PGconn* connection;

public:
	AnamnesisRepository(PGconn* conn);

	std::vector<Anamnesis> getByPatientId(int id_patient);
};