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
