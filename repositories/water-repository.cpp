#include <iostream>
#include "water-repository.h"

WaterRepository::WaterRepository(PGconn* connection) : connection(connection) {}

std::vector<Water> WaterRepository::getByUserUUID(const std::string& user_uuid) {
	std::vector<Water> patients;

	const char* query =
		"WITH ranked_water AS ("
		"	SELECT w.id_patient, w.last_water, wf.frequency, wf.frequency_measure, "
		"		ROW_NUMBER() OVER(PARTITION BY w.id_patient ORDER BY w.last_water DESC) as rn "
		"	FROM water AS w LEFT JOIN water_frequency AS wf ON w.id_patient = wf.id_patient "
		")"
		"SELECT rw.last_water, rw.frequency, rw.frequency_measure, p.id_patient "
		"FROM patient AS p "
		"LEFT JOIN ranked_water AS rw ON p.id_patient = rw.id_patient AND rw.rn = 1 "
		"WHERE p.user_uuid = $1 ORDER BY rw.last_water DESC; ";

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
		const int col_id_patient = PQfnumber(res, "id_patient");
		int col_last_water = PQfnumber(res, "last_water");
		int col_frequency = PQfnumber(res, "frequency");
		int col_frequency_measure = PQfnumber(res, "frequency_measure");


		Water water;

		water.idPatient = std::stoi(PQgetvalue(res, i, col_id_patient));
		water.lastWater = PQgetvalue(res, i, col_last_water);

		const char* val = PQgetvalue(res, i, col_frequency);
		if (val != nullptr && val[0] != '\0') {
			water.frequency = std::stoi(val);
		}
		else {
			water.frequency = -1;
		}

		water.frequencyMeasure = PQgetvalue(res, i, col_frequency_measure);
		

		patients.push_back(water);
	}
	PQclear(res);
	return patients;
}