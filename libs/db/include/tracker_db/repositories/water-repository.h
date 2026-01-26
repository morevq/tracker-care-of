#pragma once

#include <string>
#include <vector>
#include <libpq-fe.h>

#include "tracker/models/water.h"

class WaterRepository {
private:
	PGconn* connection;

public:
	WaterRepository(PGconn* conn);

	std::vector<Water> getByUserUUID(const std::string& user_uuid);
};