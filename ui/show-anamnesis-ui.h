#pragma once

#include <libpq-fe.h>
#include <string>

struct AnamnesisTableRow {
    int id;
    std::string date;
    std::string description;
};

void showAnamnesisUI(
    int patientId,
    PGconn* conn,
    const std::string& patientName
);
