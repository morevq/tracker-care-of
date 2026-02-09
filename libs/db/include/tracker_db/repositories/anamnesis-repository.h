#pragma once

#include <string>
#include <vector>
#include <libpq-fe.h>
#include <memory>
#include <tracker_db/db-utils.h>

#include "tracker/models/anamnesis.h"

class AnamnesisRepository {
private:
	db_utils::PGconnPtr connection;

public:
	AnamnesisRepository(db_utils::PGconnPtr conn);

	std::vector<Anamnesis> getByPatientId(int id_patient);
	std::optional<Anamnesis> getByID(int id_anamnesis);
	void createAnamnesis(int id_patient, std::string description, std::optional<std::string> photo_url);
	void updateAnamnesis(int id_patient, std::optional<std::string> description, std::optional<std::string> date, std::optional<std::string> photo_url);
	void deleteAnamnesis(int id_anamnesis);
};