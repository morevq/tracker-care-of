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
	void createAnamnesis(int id_patient, std::string description, std::optional<std::string> photo_url);
	void deleteAnamnesis(int id_anamnesis);
};