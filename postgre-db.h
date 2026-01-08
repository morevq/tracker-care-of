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
};
