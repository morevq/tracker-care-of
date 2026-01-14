#pragma once

#include <string>
#include <vector>
#include <cstddef>

#include "ui-common.h"
#include "../models/water.h"

struct PatientTableRow {
    int id_patient;
    std::string name;
    std::string birth_date;
    std::string age;
    Water water;
};

int interactiveTable(const std::vector<PatientTableRow>& rows);

#include "interactive-table.inl"
