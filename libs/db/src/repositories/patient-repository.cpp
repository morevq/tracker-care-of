#include <iostream>
#include "tracker_db/repositories/patient-repository.h"

PatientRepository::PatientRepository(PGconn* connection) : connection(connection) {}

std::vector<Patient> PatientRepository::getByUserUUID(const std::string& user_uuid) {
	std::vector<Patient> patients;

	const char* query =
		"SELECT id_patient, user_uuid, name, birth_date "
		"FROM patient WHERE user_uuid = $1 AND is_deleted = FALSE;";

	const char* params[] = {
		user_uuid.c_str()
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
		int col_id = PQfnumber(res, "id_patient");
		int col_user = PQfnumber(res, "user_uuid");
		int col_name = PQfnumber(res, "name");
		int col_birth_date = PQfnumber(res, "birth_date");

		Patient patient;

		patient.id_patient = std::stoi(PQgetvalue(res, i, col_id));
		patient.user_uuid = PQgetvalue(res, i, col_user);
		patient.name = PQgetvalue(res, i, col_name);
		patient.birth_date = PQgetvalue(res, i, col_birth_date);
		
		char* birth_date_cstr = PQgetvalue(res, i, col_birth_date);
		if (birth_date_cstr && birth_date_cstr[0] != '\0') {
			patient.birth_date = std::string(birth_date_cstr);
		} else {
			patient.birth_date = std::nullopt;
		}

		patients.push_back(patient);
	}
	PQclear(res);
	return patients;
}

std::optional<Patient> PatientRepository::getByID(int id_patient) {
	const char* query =
		"SELECT id_patient, user_uuid, name, birth_date "
		"FROM patient WHERE id_patient = $1 AND is_deleted = FALSE;";

	std::string id_str = std::to_string(id_patient);
	const char* params[] = { id_str.c_str() };

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
		return std::nullopt;
	}

	int rows = PQntuples(res);
	if (rows == 0) {
		PQclear(res);
		return std::nullopt;
	}

	int col_id = PQfnumber(res, "id_patient");
	int col_user = PQfnumber(res, "user_uuid");
	int col_name = PQfnumber(res, "name");
	int col_birth_date = PQfnumber(res, "birth_date");

	Patient patient;
	patient.id_patient = std::stoi(PQgetvalue(res, 0, col_id));
	patient.user_uuid = PQgetvalue(res, 0, col_user);
	patient.name = PQgetvalue(res, 0, col_name);

	char* birth_date_cstr = PQgetvalue(res, 0, col_birth_date);
	if (birth_date_cstr && birth_date_cstr[0] != '\0') {
		patient.birth_date = std::string(birth_date_cstr);
	} else {
		patient.birth_date = std::nullopt;
	}

	PQclear(res);
	return patient;
}

void PatientRepository::createPatient(const std::string& user_uuid, const std::string& name, std::optional<std::string> birth_date) {
	const char* params[3] = { user_uuid.c_str(), name.c_str(), nullptr };
	if (birth_date.has_value()) {
		params[2] = birth_date->c_str();
	}

	const char* query =
		"INSERT INTO patient (user_uuid, name, birth_date) VALUES ($1, $2, $3);";

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
		std::cerr << "Error inserting patient: " << PQerrorMessage(connection) << std::endl;
	}

	PQclear(res);
}

void PatientRepository::deletePatient(int id_patient) {
	std::string id_str = std::to_string(id_patient);
	const char* params[] = { id_str.c_str() };

	const char* query =
		"UPDATE patient SET is_deleted = TRUE WHERE id_patient = $1;";

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
		std::cerr << "Error deleting patient: " << PQerrorMessage(connection) << std::endl;
	}

	PQclear(res);
}