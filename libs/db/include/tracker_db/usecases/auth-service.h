#pragma once

#include <optional>
#include <string>

#include <userver/storages/postgres/cluster.hpp>

class AuthService {
public:
    explicit AuthService(userver::storages::postgres::ClusterPtr cluster);

    std::optional<std::string> registerUser(const std::string& email,
                                            const std::string& password);
    std::optional<std::string> loginUser(const std::string& email,
                                         const std::string& password);
    userver::storages::postgres::ClusterPtr getCluster() const;

private:
    userver::storages::postgres::ClusterPtr cluster_;
};
