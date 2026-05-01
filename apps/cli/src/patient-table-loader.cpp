#include "patient-table-loader.h"
#include "date-utils.h"

std::vector<PatientTableRow> loadPatientsTable(ApiClient& apiClient) {
    auto patientsData = apiClient.getPatients();

    std::vector<PatientTableRow> tablePatients;
    tablePatients.reserve(patientsData.size());

    for (const auto& p : patientsData) {
        Water water{};
        water.idWater = 0;
        water.idPatient = p.id;
        water.lastWater = "-";
        water.frequency = -1;
        water.frequencyMeasure = "";

        auto waters = apiClient.getWaterForPatient(p.id);
        if (!waters.empty()) {
            water.idWater = waters.front().id_water;
            water.lastWater = waters.front().lastWater;
        }

        if (auto freq = apiClient.getWaterFrequency(p.id)) {
            water.frequency = freq->frequency;
            water.frequencyMeasure = freq->measure;
        }

        std::string ageStr = calculateAge(p.birth_date);

        tablePatients.push_back(PatientTableRow{
            .id_patient = p.id,
            .name = p.name,
            .birth_date = p.birth_date.value_or("-"),
            .age = ageStr,
            .water = water
        });
    }

    return tablePatients;
}
