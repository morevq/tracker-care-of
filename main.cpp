#include <iostream>
#include <libpq-fe.h>
#include <filesystem>
#include <vector>
#include <string>
#include <optional>

#include "db/postgre-db.h"
#include "repositories/patient-repository.h"
#include "ui/interactive-table.h"

using namespace std;

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");

    try {
        PostgreDB db;
        string my_user_uuid = "5f9f079f-b158-4079-a45d-9477d2c26356";
        PGconn* conn = db.getConnection();

        PatientRepository patientRepo(conn);
        auto patientsData = patientRepo.getByUserUUID(my_user_uuid);

        if (patientsData.empty()) {
            cout << "No patients found." << endl;
            return 0;
        }

        // подготовка данных для интерактивной таблицы
        vector<PatientTableRow> tablePatients;
        for (const auto& p : patientsData) {
            PatientTableRow row;
            row.name = p.name;
			row.birthDate = p.birth_date.value_or("-");
            row.age = p.getAge();
            row.id_patient = p.id_patient;
            tablePatients.push_back(row);
        }

        int selectedId = interactiveTable(tablePatients);

        cout << "\nYou selected patient with ID: " << selectedId << endl;

    }
    catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
