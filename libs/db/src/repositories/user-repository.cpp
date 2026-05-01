#include "tracker_db/repositories/user-repository.h"

#include <userver/storages/postgres/io/optional.hpp>

namespace pg = userver::storages::postgres;

UserRepository::UserRepository(pg::ClusterPtr cluster)
    : cluster_(std::move(cluster)) {}

std::optional<User> UserRepository::getByEmail(const std::string& email) {
    auto result = cluster_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT user_uuid::text, email, password_hash "
        "FROM users WHERE email = $1 AND is_deleted = FALSE",
        email
    );

    if (result.IsEmpty()) {
        return std::nullopt;
    }

    auto row = result[0];
    User user;
    user.user_uuid     = row["user_uuid"].As<std::string>();
    user.email         = row["email"].As<std::string>();
    user.password_hash = row["password_hash"].As<std::string>();
    return user;
}

std::optional<User> UserRepository::getByUUID(const std::string& user_uuid) {
    auto result = cluster_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT user_uuid::text, email, password_hash, created_at::text "
        "FROM users WHERE user_uuid = $1::uuid AND is_deleted = FALSE",
        user_uuid
    );

    if (result.IsEmpty()) {
        return std::nullopt;
    }

    auto row = result[0];
    User user;
    user.user_uuid     = row["user_uuid"].As<std::string>();
    user.email         = row["email"].As<std::string>();
    user.password_hash = row["password_hash"].As<std::string>();
    user.created_at    = row["created_at"].As<std::string>();
    return user;
}

std::string UserRepository::createUser(const std::string& email,
                                       const std::string& passwordHash) {
    auto result = cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "INSERT INTO users (email, password_hash) VALUES ($1, $2) "
        "RETURNING user_uuid::text",
        email, passwordHash
    );

    if (result.IsEmpty()) {
        return {};
    }
    return result[0]["user_uuid"].As<std::string>();
}

void UserRepository::updateUser(const std::string& user_uuid,
                                const std::optional<std::string>& email,
                                const std::optional<std::string>& password_hash) {
    if (!email.has_value() && !password_hash.has_value()) {
        return;
    }

    cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "UPDATE users SET "
        "  email         = COALESCE($2, email), "
        "  password_hash = COALESCE($3, password_hash) "
        "WHERE user_uuid = $1::uuid AND is_deleted = FALSE",
        user_uuid, email, password_hash
    );
}

void UserRepository::deleteUser(const std::string& user_uuid) {
    cluster_->Execute(
        pg::ClusterHostType::kMaster,
        "UPDATE users SET is_deleted = TRUE WHERE user_uuid = $1::uuid",
        user_uuid
    );
}
