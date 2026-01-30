#pragma once

#include <string>
#include "api-client.h"

struct AnamnesisTableRow {
    int id;
    std::string date;
    std::string description;
};

void showAnamnesisUI(
    int patientId,
    ApiClient& apiClient,
    const std::string& patientName
);
