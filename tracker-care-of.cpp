#include <iostream>
#include "postgre-db.h"
using namespace std;

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    try {
        PostgreDB db;

        int patient_id = db.create_patient(
            "5f9f079f-b158-4079-a45d-9477d2c26356",
            "Фикус Бенджамина",
            "2022-04-10"
        );

        cout << "Patient created, id = " << patient_id << endl;

        db.set_water_frequency(
            patient_id,
            "3",
            "days"
        );

        cout << "Watering frequency set\n";

        db.add_water_event(patient_id);

        cout << "Water event added\n";

        db.add_anamnesis(
            patient_id,
            "Пересадка в новый горшок"
        );

        cout << "Anamnesis added\n";
    }
    catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
