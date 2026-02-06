#pragma once

#include <memory>
#include <libpq-fe.h>

namespace db_utils {

    struct PGconnDeleter {
        void operator()(PGconn* conn) const {
            if (conn) {
                PQfinish(conn);
            }
        }
    };

    struct PGresultDeleter {
        void operator()(PGresult* res) const {
            if (res) {
                PQclear(res);
            }
        }
    };

    using PGconnPtr = std::shared_ptr<PGconn>;
    using PGresultPtr = std::unique_ptr<PGresult, PGresultDeleter>;

    inline PGconnPtr make_pgconn(const char* conninfo) {
        return PGconnPtr(PQconnectdb(conninfo), PGconnDeleter());
    }

    inline PGresultPtr make_pgresult(PGresult* res) {
        return PGresultPtr(res, PGresultDeleter());
    }

}
