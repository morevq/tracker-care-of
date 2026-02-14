#include <iostream>
#include "tracker_db/repositories/water-repository.h"

WaterRepository::WaterRepository(db_utils::PGconnPtr connection) : connection(connection) {}

std::vector<Water> WaterRepository::getByUserUUID(const std::string& user_uuid) {
	std::vector<Water> patients;

	const char* query =
		R"(SELECT
			  w.last_water,
			  wf.frequency,
			  wf.frequency_measure,
			  p.id_patient
		  FROM patient p
		  LEFT JOIN water w ON w.id_patient = p.id_patient
		  LEFT JOIN water_frequency wf ON wf.id_patient = p.id_patient
		  WHERE p.user_uuid = $1
		  ORDER BY w.last_water DESC NULLS LAST;)";

	const char* params[] = {
		user_uuid.c_str()
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
		const int col_id_patient = PQfnumber(res.get(), "id_patient");
		int col_last_water = PQfnumber(res.get(), "last_water");
		int col_frequency = PQfnumber(res.get(), "frequency");
		int col_frequency_measure = PQfnumber(res.get(), "frequency_measure");


		Water water;

		water.idPatient = std::stoi(PQgetvalue(res.get(), i, col_id_patient));
		water.lastWater = PQgetvalue(res.get(), i, col_last_water);

		const char* val = PQgetvalue(res.get(), i, col_frequency);
		if (val != nullptr && val[0] != '\0') {
			water.frequency = std::stoi(val);
		}
		else {
			water.frequency = -1;
		}

		water.frequencyMeasure = PQgetvalue(res.get(), i, col_frequency_measure);
		

		patients.push_back(water);
	}
	return patients;
}

std::optional<Water> WaterRepository::getByPatientID(int id_patient) {
	const char* query =
		"SELECT w.id_patient, w.last_water, wf.frequency, wf.frequency_measure "
		"FROM water AS w "
		"LEFT JOIN water_frequency AS wf ON w.id_patient = wf.id_patient "
		"WHERE w.id_patient = $1 "
		"ORDER BY w.last_water DESC "
		"LIMIT 1;";

	std::string id_str = std::to_string(id_patient);
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
		std::cerr << "Error executing query: " << PQerrorMessage(connection.get()) << std::endl;
		return std::nullopt;
	}

	int rows = PQntuples(res.get());
	if (rows == 0) {
		return std::nullopt;
	}

	int col_id_patient = PQfnumber(res.get(), "id_patient");
	int col_last_water = PQfnumber(res.get(), "last_water");
	int col_frequency = PQfnumber(res.get(), "frequency");
	int col_frequency_measure = PQfnumber(res.get(), "frequency_measure");

	Water water;
	water.idPatient = std::stoi(PQgetvalue(res.get(), 0, col_id_patient));
	water.lastWater = PQgetvalue(res.get(), 0, col_last_water);

	const char* val = PQgetvalue(res.get(), 0, col_frequency);
	if (val != nullptr && val[0] != '\0') {
		water.frequency = std::stoi(val);
	}
	else {
		water.frequency = -1;
	}

	water.frequencyMeasure = PQgetvalue(res.get(), 0, col_frequency_measure);

	return water;
}

bool WaterRepository::addWater(int id_patient, const std::string& last_water) {
	std::string id_str = std::to_string(id_patient);
	const char* params_check[] = { id_str.c_str() };
	const char* query_check = 
		"SELECT birth_date FROM patient WHERE id_patient = $1 AND is_deleted = FALSE;";

	auto res_check = db_utils::make_pgresult(PQexecParams(
		connection.get(),
		query_check,
		1,
		nullptr,
		params_check,
		nullptr,
		nullptr,
		0
	));

	if (PQresultStatus(res_check.get()) != PGRES_TUPLES_OK || PQntuples(res_check.get()) == 0) {
		return false;
	}

	if (!PQgetisnull(res_check.get(), 0, 0)) {
		std::string birth_date = PQgetvalue(res_check.get(), 0, 0);
		if (!birth_date.empty() && last_water < birth_date) {
			return false;
		}
	}

	const char* params[] = { id_str.c_str(), last_water.c_str() };

	auto exec_cmd = [&](const char* q, int nParams, const char* const* p) -> bool {
		auto res = db_utils::make_pgresult(PQexecParams(
			connection.get(), q, nParams, nullptr, p, nullptr, nullptr, 0
		));
		auto st = PQresultStatus(res.get());
		if (!(st == PGRES_COMMAND_OK || st == PGRES_TUPLES_OK)) {
			std::cerr << "PG error: " << PQerrorMessage(connection.get()) << std::endl;
			return false;
		}
		return true;
	};

	exec_cmd("INSERT INTO water (id_patient, last_water) VALUES ($1, $2);", 2, params);
	return true;
}

void WaterRepository::deleteWater(int id_patient) {
	std::string id_str = std::to_string(id_patient);
	const char* params[] = { id_str.c_str() };
	const char* query = "DELETE FROM water WHERE id_patient = $1;";

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
		std::cerr << "Error deleting water: " << PQerrorMessage(connection.get()) << std::endl;
	}
}