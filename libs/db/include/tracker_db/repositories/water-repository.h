#pragma once

#include <string>
#include <vector>
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
	void deleteWater(int id_patient);
};