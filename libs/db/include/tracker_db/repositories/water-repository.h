#pragma once

#include <string>
#include <vector>
#include <optional>
#include <libpq-fe.h>
#include <memory>
#include <tracker_db/db-utils.h>

#include "tracker/models/water.h"

class WaterRepository {
private:
	db_utils::PGconnPtr connection;

public:
	WaterRepository(db_utils::PGconnPtr conn);

	std::vector<Water> getByUserUUID(const std::string& user_uuid);
	std::optional<Water> getByPatientID(int id_patient);
	bool addWater(int id_patient, const std::string& last_water);
	void deleteWater(int id_patient);
};