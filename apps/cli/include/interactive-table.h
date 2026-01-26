#pragma once

#include <string>
#include <vector>
#include <cstddef>

#include "ui-common.h"
#include "tracker/models/water.h"

struct PatientTableRow {
    int id_patient;
    std::string name;
    std::string birth_date;
    std::string age;
    Water water;
};

int interactiveTable(const std::vector<PatientTableRow>& rows, int& selectedIndex);

#include "interactive-table.inl"
