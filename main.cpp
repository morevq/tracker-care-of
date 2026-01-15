#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <unordered_map>

#include <libpq-fe.h>

#include "db/postgre-db.h"
#include "repositories/patient-repository.h"
#include "repositories/water-repository.h"
#include "ui/interactive-table.h"
#include "ui/show-anamnesis-ui.h"

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");

    try {
        PostgreDB db;
        std::string my_user_uuid = "5f9f079f-b158-4079-a45d-9477d2c26356";

        PGconn* conn = db.getConnection();

        PatientRepository patientRepo(conn);
        auto patientsData = patientRepo.getByUserUUID(my_user_uuid);

        WaterRepository waterRepo(conn);
        const auto waterData = waterRepo.getByUserUUID(my_user_uuid);

        std::unordered_map<int, Water> waterByPatientId;
        waterByPatientId.reserve(waterData.size());
        for (const auto& w : waterData) {
            waterByPatientId.emplace(w.idPatient, w);
        }

        if (patientsData.empty()) {
            std::cout << "No patients found.\n";
            return 0;
        }

        std::vector<PatientTableRow> tablePatients;
        tablePatients.reserve(patientsData.size());

        for (const auto& p : patientsData) {
            Water water{};
            water.idPatient = p.id_patient;
            water.lastWater = "-";
            water.frequency = -1;
            water.frequencyMeasure = "";

            const auto itWater = waterByPatientId.find(p.id_patient);
            if (itWater != waterByPatientId.end()) {
                water = itWater->second;
            }

            tablePatients.push_back(PatientTableRow{
                .id_patient = p.id_patient,
                .name = p.name,
                .birth_date = p.birth_date.value_or("-"),
                .age = p.getAge(),
                .water = water
            });
        }

        int selectedIndex = 0;
        while (true) {
            int selectedId = interactiveTable(tablePatients, selectedIndex);

            if (selectedId == -1)
                break;

            auto it = std::find_if(
                tablePatients.begin(),
                tablePatients.end(),
                [&](const PatientTableRow& r) {
                    return r.id_patient == selectedId;
                }
            );

            if (it != tablePatients.end()) {
                showAnamnesisUI(
                    it->id_patient,
                    conn,
                    it->name
                );
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
