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

    std::string RedisSessionStore::makeKey(const std::string& sid) {
        return "sess:" + sid;
    }

    std::string RedisSessionStore::createSession(const std::string& userUuid, int ttlSeconds) {
        if (ttlSeconds <= 0) throw std::invalid_argument("ttlSeconds must be > 0");

        std::string sid = secureRandomHex(32);
        std::string key = makeKey(sid);

        client_->Set(key, userUuid, redis::CommandControl{}).Get();
        client_->Expire(key, std::chrono::seconds(ttlSeconds), redis::CommandControl{}).Get();

        return sid;
    }

    std::optional<std::string> RedisSessionStore::resolveUserUuid(const std::string& sid) {
        if (sid.empty()) return std::nullopt;

        auto reply = client_->Get(makeKey(sid), redis::CommandControl{}).Get();
        if (!reply) return std::nullopt;
        return *reply;
    }

    void RedisSessionStore::destroySession(const std::string& sid) {
        if (sid.empty()) return;
        client_->Del(makeKey(sid), redis::CommandControl{}).Get();
    }

}
