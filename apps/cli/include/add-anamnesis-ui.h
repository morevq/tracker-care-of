#pragma once

#include <libpq-fe.h>

bool addAnamnesisUI(int patient_id, PGconn* connection);
