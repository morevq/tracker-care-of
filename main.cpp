#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <algorithm>

#include <libpq-fe.h>

#include "db/postgre-db.h"
#include "repositories/patient-repository.h"
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

        if (patientsData.empty()) {
            std::cout << "No patients found.\n";
            return 0;
        }

        std::vector<PatientTableRow> tablePatients;
        tablePatients.reserve(patientsData.size());

        for (const auto& p : patientsData) {
            tablePatients.push_back(PatientTableRow{
                .id_patient = p.id_patient,
                .name = p.name,
                .birth_date = p.birth_date.value_or("-"),
                .age = p.getAge()
                });
        }

        while (true) {
            int selectedId = interactiveTable(tablePatients);

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
