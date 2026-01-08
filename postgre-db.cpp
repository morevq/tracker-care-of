#include <iostream>

#include "postgre-db.h"
#include "env-parser.h"

void PostgreDB::check_connection() {
    if (PQstatus(connection) == CONNECTION_OK) {
        std::cout << "Successful connection to DB\n";
    }
    else {
        std::cerr << "Error: " << PQerrorMessage(connection) << std::endl;
    }
}

PostgreDB::PostgreDB() {
    auto env = load_env(".env");

    const char* keywords[] = {
        "host", "port", "dbname", "user", "password", nullptr
    };

    const char* values[] = {
        env.at("DB_HOST").c_str(),
        env.at("DB_PORT").c_str(),
        env.at("DB_NAME").c_str(),
        env.at("DB_USER").c_str(),
        env.at("DB_PASSWORD").c_str(),
        nullptr
    };

    connection = PQconnectdbParams(keywords, values, 0);
    check_connection();
}

PostgreDB::~PostgreDB() {
    PQfinish(connection);
    std::cout << "Connection closed\n";
}


int PostgreDB::create_patient(
    const std::string& user_uuid,
    const std::string& name,
    const std::string& birth_date
) {
    const char* query = 
        "INSERT INTO patient (user_uuid, name, birth_date) "
		"VALUES ($1, $2, $3) RETURNING id_patient;";

    const char* params[] = {
        user_uuid.c_str(),
        name.c_str(),
        birth_date.c_str()
	};

    PGresult* res = PQexecParams(
        connection,
        query,
        3,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Error executing query: " << PQerrorMessage(connection) << std::endl;
        PQclear(res);
        return -1;
    }

    int id_patient = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);

	return id_patient;
}

void PostgreDB::add_anamnesis(
    int id_patient,
    const std::string& description,
    const std::string& photo_url
) {
    const char* query = 
        "INSERT INTO anamnesis (id_patient, description, photo_url) "
        "VALUES ($1, $2, $3);";

    std::string id_str = std::to_string(id_patient);

    const char* params[] = {
        id_str.c_str(),
        description.c_str(),
		photo_url.empty() ? nullptr : photo_url.c_str()
    };

    PGresult* res = PQexecParams(
        connection,
        query,
        3,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Error executing query: " << PQerrorMessage(connection) << std::endl;
    }

    PQclear(res);
}

void PostgreDB::add_water_event(int id_patient) {
    const char* query = 
        "INSERT INTO water (id_patient) VALUES ($1);";

    std::string id_str = std::to_string(id_patient);

    const char* params[] = {
        id_str.c_str()
    };

    PGresult* res = PQexecParams(
        connection,
        query,
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Error executing query: " << PQerrorMessage(connection) << std::endl;
    }

    PQclear(res);
}

void PostgreDB::set_water_frequency(
    int id_patient,
    const std::string& frequency,
    const std::string& measure
) {
    const char* query = 
        "UPDATE water_frequency SET frequency = $1, frequency_measure = $2 "
        "WHERE id_patient = $3;";

	std::string id_str = std::to_string(id_patient);

    const char* params[] = {
        frequency.c_str(),
        measure.c_str(),
        id_str.c_str()
    };

    PGresult* res = PQexecParams(
        connection,
        query,
        3,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Error executing query: " << PQerrorMessage(connection) << std::endl;
    }

    PQclear(res);
}