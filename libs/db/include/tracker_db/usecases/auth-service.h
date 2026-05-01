#pragma once

#include <optional>
#include <string>

#include <userver/storages/postgres/cluster.hpp>

class AuthService {
public:
    enum class ChangePasswordResult {
        kOk,
        kUserNotFound,
        kWrongCurrentPassword,
        kInvalidNewPassword,
    };

    explicit AuthService(userver::storages::postgres::ClusterPtr cluster);

    std::optional<std::string> registerUser(const std::string& email,
                                            const std::string& password);
    std::optional<std::string> loginUser(const std::string& email,
                                         const std::string& password);
    ChangePasswordResult changePassword(const std::string& user_uuid,
                                        const std::string& current_password,
                                        const std::string& new_password);
    userver::storages::postgres::ClusterPtr getCluster() const;

private:
    userver::storages::postgres::ClusterPtr cluster_;
};
