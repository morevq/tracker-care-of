#include "tracker_session/redis-session-store.h"
#include "tracker_session/secure-random.h"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace redis = userver::storages::redis;

namespace tracker_session {

    RedisSessionStore::RedisSessionStore(
        std::shared_ptr<userver::storages::redis::Client> client)
        : client_(std::move(client)) {
    }

    std::string RedisSessionStore::makeSessionKey(const std::string& sid) {
        return "sess:" + sid;
    }

    std::string RedisSessionStore::makeUserKey(const std::string& userUuid) {
        return "user_sessions:" + userUuid;
    }

    std::string RedisSessionStore::createSession(const std::string& userUuid, int ttlSeconds) {
        if (ttlSeconds <= 0) throw std::invalid_argument("ttlSeconds must be > 0");

        std::string sid = secureRandomHex(32);
        std::string sessKey = makeSessionKey(sid);
        std::string userKey = makeUserKey(userUuid);

        client_->Set(sessKey, userUuid, redis::CommandControl{}).Get();
        client_->Expire(sessKey, std::chrono::seconds(ttlSeconds), redis::CommandControl{}).Get();
        client_->Sadd(userKey, sid, redis::CommandControl{}).Get();

        return sid;
    }

    std::optional<std::string> RedisSessionStore::resolveUserUuid(const std::string& sid) {
        if (sid.empty()) return std::nullopt;

        auto reply = client_->Get(makeSessionKey(sid), redis::CommandControl{}).Get();
        if (!reply) return std::nullopt;
        return *reply;
    }

    void RedisSessionStore::destroySession(const std::string& sid) {
        if (sid.empty()) return;

        const auto sessKey = makeSessionKey(sid);
        auto userUuid = client_->Get(sessKey, redis::CommandControl{}).Get();
        client_->Del(sessKey, redis::CommandControl{}).Get();
        if (userUuid) {
            client_->Srem(makeUserKey(*userUuid), sid, redis::CommandControl{}).Get();
        }
    }

    void RedisSessionStore::destroyAllSessionsForUser(const std::string& userUuid) {
        if (userUuid.empty()) return;

        const auto userKey = makeUserKey(userUuid);
        auto members = client_->Smembers(userKey, redis::CommandControl{}).Get();
        for (const auto& sid : members) {
            client_->Del(makeSessionKey(sid), redis::CommandControl{}).Get();
        }
        client_->Del(userKey, redis::CommandControl{}).Get();
    }

}
