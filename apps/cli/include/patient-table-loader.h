#pragma once

#include <vector>
#include "interactive-table.h"
#include "api-client.h"

std::vector<PatientTableRow> loadPatientsTable(ApiClient& apiClient);
