#pragma once

#include <libpq-fe.h>
#include <string>
#include <unordered_map>

class PostgreDB {
private:
    PGconn* connection;
    void check_connection();

public:
    PostgreDB();
    ~PostgreDB();

    int create_patient(
        const std::string& user_uuid,
        const std::string& name,
        const std::string& birth_date
    );

    void add_anamnesis(
        int id_patient,
        const std::string& description,
        const std::string& photo_url = ""
    );

    void add_water_event(int id_patient);

    void set_water_frequency(
        int id_patient,
        const std::string& frequency,
        const std::string& measure
    );

    PGconn* getConnection();
};
