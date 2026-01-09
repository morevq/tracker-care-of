#include <iostream>
#include "patient-repository.h"

PatientRepository::PatientRepository(PGconn* conn) : connection(conn) {}

std::vector<Patient> PatientRepository::getByUserUUID(const std::string& user_uuid) {
	std::vector<Patient> patients;

	const char* query =
		"SELECT id_patient, user_uuid, name, birth_date "
		"FROM patient WHERE user_uuid = $1;";

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
		patient.id_user = PQgetvalue(res, i, col_user);
		patient.name = PQgetvalue(res, i, col_name);
		patient.birth_date = PQgetvalue(res, i, col_birth_date);
		
		char* birth_date_cstr = PQgetvalue(res, i, 3);
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