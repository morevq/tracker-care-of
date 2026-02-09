#include <iostream>
#include <stdexcept>
#include <vector>
#include "tracker_db/repositories/anamnesis-repository.h"

AnamnesisRepository::AnamnesisRepository(db_utils::PGconnPtr connection) : connection(connection) {}

std::vector<Anamnesis> AnamnesisRepository::getByPatientId(int id_patient) {
	std::vector<Anamnesis> patients;

	const char* query =
		"SELECT id_anamnesis, description, date, photo_url "
		"FROM anamnesis WHERE id_patient = $1 AND is_deleted = FALSE;";

	std::string id_str = std::to_string(id_patient);
	const char* params[] = {
		id_str.c_str()
	};

	auto res = db_utils::make_pgresult(PQexecParams(
		connection.get(),
		query,
		1,
		nullptr,
		params,
		nullptr,
		nullptr,
		0
	));

	if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
		std::cerr << "Error executing query: " << PQerrorMessage(connection.get()) << std::endl;
		return patients;
	}

	int rows = PQntuples(res.get());
	for (int i = 0; i < rows; ++i) {
		int col_id = PQfnumber(res.get(), "id_anamnesis");
		int col_description = PQfnumber(res.get(), "description");
		int col_date = PQfnumber(res.get(), "date");
		int col_photo_url = PQfnumber(res.get(), "photo_url");

		Anamnesis anamnesis;

		anamnesis.id = std::stoi(PQgetvalue(res.get(), i, col_id));
		anamnesis.id_patient = id_patient;
		anamnesis.description = PQgetvalue(res.get(), i, col_description);
		anamnesis.created_at = PQgetvalue(res.get(), i, col_date);

		char* photo_url_cstr = PQgetvalue(res.get(), i, col_photo_url);
		if (photo_url_cstr && photo_url_cstr[0] != '\0') {
			anamnesis.photo_url = std::string(photo_url_cstr);
		}
		else {
			anamnesis.photo_url = std::nullopt;
		}

		patients.push_back(anamnesis);
	}
	return patients;
}

std::optional<Anamnesis> AnamnesisRepository::getByID(int id_anamnesis) {
	const char* query = 
		"SELECT id_anamnesis, id_patient, description, photo_url, date "
		"FROM anamnesis WHERE id_anamnesis = $1 AND is_deleted = FALSE;";

	std::string id_str = std::to_string(id_anamnesis);
	const char* params[] = { id_str.c_str() };

	auto res = db_utils::make_pgresult(PQexecParams(
		connection.get(),
		query,
		1,
		nullptr,
		params,
		nullptr,
		nullptr,
		0
	));

	if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
		throw std::runtime_error("Failed to get anamnesis: " + std::string(PQerrorMessage(connection.get())));
	}

	if (PQntuples(res.get()) == 0) {
		return std::nullopt;
	}

	Anamnesis anamnesis;
	anamnesis.id = std::atoi(PQgetvalue(res.get(), 0, 0));
	anamnesis.id_patient = std::atoi(PQgetvalue(res.get(), 0, 1));
	anamnesis.description = PQgetvalue(res.get(), 0, 2);
	anamnesis.photo_url = PQgetisnull(res.get(), 0, 3) ? std::nullopt : std::optional<std::string>(PQgetvalue(res.get(), 0, 3));
	anamnesis.created_at = PQgetvalue(res.get(), 0, 4);

	return anamnesis;
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

	auto res = db_utils::make_pgresult(PQexecParams(
		connection.get(),
		query,
		3,
		nullptr,
		params,
		nullptr,
		nullptr,
		0
	));

	if (PQresultStatus(res.get()) != PGRES_COMMAND_OK) {
		std::cerr << "Error inserting anamnesis: " << PQerrorMessage(connection.get()) << std::endl;
	}
}

void AnamnesisRepository::updateAnamnesis(int id_anamnesis, std::optional<std::string> description, std::optional<std::string> date, std::optional<std::string> photo_url) {
	std::vector<std::string> setClauses;
	std::vector<const char*> params;
	int paramIndex = 1;

	if (description.has_value()) {
		setClauses.push_back("description = $" + std::to_string(paramIndex++));
		params.push_back(description->c_str());
	}

	if (date.has_value()) {
		setClauses.push_back("date = $" + std::to_string(paramIndex++));
		params.push_back(date->c_str());
	}

	if (photo_url.has_value()) {
		setClauses.push_back("photo_url = $" + std::to_string(paramIndex++));
		params.push_back(photo_url->c_str());
	}

	if (setClauses.empty()) {
		std::cerr << "No fields to update for anamnesis ID " << id_anamnesis << std::endl;
		return;
	}

	std::string id_str = std::to_string(id_anamnesis);
	params.push_back(id_str.c_str());

	std::string query = "UPDATE anamnesis SET " + std::string(setClauses[0]);

	for (size_t i = 1; i < setClauses.size(); ++i) {
		query += ", " + setClauses[i];
	}

	query += " WHERE id_anamnesis = $" + std::to_string(paramIndex) + ";";

	PGresult* res = PQexecParams(
		connection.get(),
		query.c_str(),
		static_cast<int>(params.size()),
		nullptr,
		params.data(),
		nullptr,
		nullptr,
		0
	);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		std::cerr << "Error updating anamnesis: " << PQerrorMessage(connection.get()) << std::endl;
	}

	PQclear(res);
}

void AnamnesisRepository::deleteAnamnesis(int id_anamnesis) {
	const char* query = 
		"UPDATE anamnesis SET is_deleted = TRUE WHERE id_anamnesis = $1;";

	std::string id_str = std::to_string(id_anamnesis);
	const char* params[] = {
		id_str.c_str()
	};

	auto res = db_utils::make_pgresult(PQexecParams(
		connection.get(),
		query,
		1,
		nullptr,
		params,
		nullptr,
		nullptr,
		0
	));

	if (PQresultStatus(res.get()) != PGRES_COMMAND_OK) {
		std::cerr << "Error deleting anamnesis: " << PQerrorMessage(connection.get()) << std::endl;
	}
}