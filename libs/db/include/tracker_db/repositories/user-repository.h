#pragma once

#include <optional>
#include <string>

#include <userver/storages/postgres/cluster.hpp>

#include "tracker/models/user.h"

class UserRepository {
public:
    explicit UserRepository(userver::storages::postgres::ClusterPtr cluster);

    std::optional<User> getByEmail(const std::string& email);
    std::optional<User> getByUUID(const std::string& user_uuid);

    std::string createUser(const std::string& email,
                           const std::string& passwordHash);
    void updateUser(const std::string& user_uuid,
                    const std::optional<std::string>& email,
                    const std::optional<std::string>& password_hash);
    void deleteUser(const std::string& user_uuid);

private:
    userver::storages::postgres::ClusterPtr cluster_;
};
