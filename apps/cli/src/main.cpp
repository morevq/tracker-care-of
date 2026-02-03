#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <conio.h>

#include "api-client.h"
#include "auth-handler.h"
#include "patient-table-loader.h"
#include "console-utils.h"
#include "tracker_common/env-parser.h"
#include "interactive-table.h"
#include "show-anamnesis-ui.h"
#include "add-patient-ui.h"

constexpr int PATIENT_DELETE = -3;
constexpr int PATIENT_ADD_NEW = -2;
constexpr int PATIENT_EXIT = -1;

int main() {
setlocale(LC_ALL, "ru_RU.UTF-8");

try {
    auto env = load_env(".env");
    std::string apiUrl = env.count("API_URL") ? env["API_URL"] : "http://localhost:8080";

    ApiClient apiClient(apiUrl);

    if (!AuthHandler::authenticate(apiClient)) {
        return 1;
    }

        auto tablePatients = loadPatientsTable(apiClient);
        int selectedIndex = 0;

        while (true) {
            if (tablePatients.empty()) {
                clearScreen();
                std::cout << "No patients found.\n";
                std::cout << "Press A to add patient, Esc to exit\n";

                const InputAction a = getInput();
                if (a == InputAction::Escape) break;
                if (a == InputAction::Add) {
                    if (addPatientUI(apiClient)) {
                        tablePatients = loadPatientsTable(apiClient);
                        selectedIndex = 0;
                    }
                }
                continue;
            }

            int selectedId = interactiveTable(tablePatients, selectedIndex);

            if (selectedId == PATIENT_DELETE) {
                clearScreen();
                std::cout << "Delete patient: " << tablePatients[selectedIndex].name << "?\n";
                std::cout << "Press Y to confirm, any other key to cancel\n";

                int ch = _getch();
                if (ch == 'y' || ch == 'Y') {
                    apiClient.deletePatient(tablePatients[selectedIndex].id_patient);
                    tablePatients = loadPatientsTable(apiClient);
                    selectedIndex = std::min<int>(selectedIndex, static_cast<int>(tablePatients.size()) - 1);
                }
                continue;
            }

            if (selectedId == PATIENT_ADD_NEW) {
                if (addPatientUI(apiClient)) {
                    tablePatients = loadPatientsTable(apiClient);
                    selectedIndex = 0;
                }
                continue;
            }

            if (selectedId == PATIENT_EXIT) {
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
                    apiClient,
                    it->name
                );

                tablePatients = loadPatientsTable(apiClient);
                selectedIndex = std::min<int>(selectedIndex, static_cast<int>(tablePatients.size()) - 1);
            }
        }

        apiClient.logout();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}