#include <iostream>
#include "tracker_db/repositories/anamnesis-repository.h"

AnamnesisRepository::AnamnesisRepository(PGconn* connection) : connection(connection) {}

std::vector<Anamnesis> AnamnesisRepository::getByPatientId(int id_patient) {
	std::vector<Anamnesis> patients;

	const char* query =
		"SELECT id_anamnesis, description, date, photo_url "
		"FROM anamnesis WHERE id_patient = $1 AND is_deleted = FALSE;";

	std::string id_str = std::to_string(id_patient);
	const char* params[] = {
		id_str.c_str()
	};

	PGresult* res = PQexecParams(
		connection,
		query,
		1,
		nullptr,
		params,
		nullptr,
		nullptr,
		0
	);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		std::cerr << "Error executing query: " << PQerrorMessage(connection) << std::endl;
		PQclear(res);
		return patients;
	}

	int rows = PQntuples(res);
	for (int i = 0; i < rows; ++i) {
		int col_id = PQfnumber(res, "id_anamnesis");
		int col_description = PQfnumber(res, "description");
		int col_date = PQfnumber(res, "date");
		int col_photo_url = PQfnumber(res, "photo_url");

		Anamnesis anamnesis;

		anamnesis.id = std::stoi(PQgetvalue(res, i, col_id));
		anamnesis.id_patient = id_patient;
		anamnesis.description = PQgetvalue(res, i, col_description);
		anamnesis.created_at = PQgetvalue(res, i, col_date);

		char* photo_url_cstr = PQgetvalue(res, i, col_photo_url);
		if (photo_url_cstr && photo_url_cstr[0] != '\0') {
			anamnesis.photo_url = std::string(photo_url_cstr);
		}
		else {
			anamnesis.photo_url = std::nullopt;
		}

		patients.push_back(anamnesis);
	}
	PQclear(res);
	return patients;
}

void AnamnesisRepository::createAnamnesis(int id_patient, std::string description, std::optional<std::string> photo_url) {
	std::string id_str = std::to_string(id_patient);
	const char* params[3] = { id_str.c_str(), description.c_str() };
	if (!photo_url.has_value()) {
		params[2] = nullptr;
	}
	else {
		params[2] = photo_url->c_str();
	}

	const char* query =
		"INSERT INTO anamnesis (id_patient, description, photo_url) VALUES ($1, $2, $3);";

	PGresult* res = PQexecParams(
		connection,
		query,
		3,
		nullptr,
		params,
		nullptr,
		nullptr,
		0
	);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		std::cerr << "Error inserting anamnesis: " << PQerrorMessage(connection) << std::endl;
	}

	PQclear(res);
}

void AnamnesisRepository::deleteAnamnesis(int id_anamnesis) {
	const char* query = 
		"UPDATE anamnesis SET is_deleted = TRUE WHERE id_patient = $1;";

	std::string id_str = std::to_string(id_anamnesis);
	const char* params[] = {
		id_str.c_str()
	};

	PGresult* res = PQexecParams(
		connection,
		query,
		1,
		nullptr,
		params,
		nullptr,
		nullptr,
		0
	);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		std::cerr << "Error deleting anamnesis: " << PQerrorMessage(connection) << std::endl;
	}

	PQclear(res);
}