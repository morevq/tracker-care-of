#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <unordered_map>

#include <libpq-fe.h>

#include "tracker_db/postgres-db.h"
#include "tracker_db/repositories/patient-repository.h"
#include "tracker_db/repositories/water-repository.h"
#include "tracker/usecases/auth-service.h"
#include "interactive-table.h"
#include "show-anamnesis-ui.h"
#include "add-patient-ui.h"
#include "console-utils.h"

static std::vector<PatientTableRow> loadPatientsTable(
    PGconn* conn,
    const std::string& userUuid
) {
    PatientRepository patientRepo(conn);
    auto patientsData = patientRepo.getByUserUUID(userUuid);

    WaterRepository waterRepo(conn);
    const auto waterData = waterRepo.getByUserUUID(userUuid);

    std::unordered_map<int, Water> waterByPatientId;
    waterByPatientId.reserve(waterData.size());
    for (const auto& w : waterData) {
        waterByPatientId.emplace(w.idPatient, w);
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

    return tablePatients;
}

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");

    try {
        PostgreDB db;
        PGconn* conn = db.getConnection();

        AuthService auth(conn);

        std::cout << "1) Register\n2) Login\n> ";
        int mode = 0;
        std::cin >> mode;

        std::string email;
        std::string password;

        std::cout << "Email: ";
        std::cin >> email;
        std::cout << "Password: ";
		password = readPassword();

        std::optional<std::string> my_user_uuid;
        if (mode == 1) {
            my_user_uuid = auth.registerUser(email, password);
        }
        else {
            my_user_uuid = auth.loginUser(email, password);
        }

        if (!my_user_uuid.has_value()) {
            std::cerr << "Auth failed.\n";
            return 1;
        }

        const std::string my_user_uuid_str = *my_user_uuid;

        auto tablePatients = loadPatientsTable(conn, my_user_uuid_str);
        int selectedIndex = 0;

        while (true) {
            if (tablePatients.empty()) {
                system("cls");
                std::cout << "No patients found.\n";
                std::cout << "Press A to add patient, Esc to exit\n";

                const InputAction a = getInput();
                if (a == InputAction::Escape) break;
                if (a == InputAction::Add) {
                    if (addPatientUI(my_user_uuid_str, conn)) {
                        tablePatients = loadPatientsTable(conn, my_user_uuid_str);
                        selectedIndex = 0;
                    }
                }
                continue;
            }

            int selectedId = interactiveTable(tablePatients, selectedIndex);

            if (selectedId == -2) {
                if (addPatientUI(my_user_uuid_str, conn)) {
                    tablePatients = loadPatientsTable(conn, my_user_uuid_str);
                    selectedIndex = 0;
                }
                continue;
            }

            if (selectedId == -1) {
				break;
            }

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

                tablePatients = loadPatientsTable(conn, my_user_uuid_str);
                selectedIndex = std::min<int>(selectedIndex, static_cast<int>(tablePatients.size()) - 1);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}