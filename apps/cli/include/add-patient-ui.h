#pragma once

#include <libpq-fe.h>
#include <string>

bool addPatientUI(const std::string& user_uuid, PGconn* connection);
