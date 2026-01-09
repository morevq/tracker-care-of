#include <iostream>
#include <libpq-fe.h>

#include <filesystem>

#include "db/postgre-db.h"
#include "repositories/patient-repository.h"
using namespace std;

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    try {
        PostgreDB db;
        string my_user_uuid = "5f9f079f-b158-4079-a45d-9477d2c26356";
		PGconn* conn = db.get_connection();

        PatientRepository patientRepo(conn);

        auto patients = patientRepo.getByUserUUID(my_user_uuid);
		cout << patientRepo.getByUserUUID(my_user_uuid).size() << " patients found:\n";
        for (const auto& p : patients) {
            std::cout << p.name << '\n';
        }
        
    }
    catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
