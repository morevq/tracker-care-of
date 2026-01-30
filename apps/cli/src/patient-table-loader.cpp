#include "patient-table-loader.h"
#include "date-utils.h"
#include <unordered_map>

std::vector<PatientTableRow> loadPatientsTable(ApiClient& apiClient) {
    auto patientsData = apiClient.getPatients();
    auto waterData = apiClient.getWaterData();

    std::unordered_map<int, ApiClient::WaterDto> waterByPatientId;
    for (const auto& w : waterData) {
        waterByPatientId.emplace(w.id, w);
    }

    std::vector<PatientTableRow> tablePatients;
    tablePatients.reserve(patientsData.size());

    for (const auto& p : patientsData) {
        Water water{};
        water.idPatient = p.id;
        water.lastWater = "-";
        water.frequency = -1;
        water.frequencyMeasure = "";

        const auto itWater = waterByPatientId.find(p.id);
        if (itWater != waterByPatientId.end()) {
            water.idPatient = itWater->second.id;
            water.lastWater = itWater->second.lastWater;
            water.frequency = itWater->second.frequency;
            water.frequencyMeasure = itWater->second.frequencyMeasure;
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
