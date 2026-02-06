#include "tracker_session/redis-session-store.h"
#include "tracker_session/secure-random.h"

#include <stdexcept>

namespace tracker_session {

    RedisSessionStore::RedisSessionStore(const std::string& redisUri)
        : redis_(redisUri) {
    }

    std::string RedisSessionStore::makeKey(const std::string& sid) {
        return "sess:" + sid;
    }

    std::string RedisSessionStore::createSession(const std::string& userUuid, int ttlSeconds) {
        if (ttlSeconds <= 0) throw std::invalid_argument("ttlSeconds must be > 0");

        std::string sid = secureRandomHex(32);
        std::string key = makeKey(sid);

        redis_.hset(key, "user_uuid", userUuid);

        redis_.expire(key, ttlSeconds);

        return sid;
    }

    std::optional<std::string> RedisSessionStore::resolveUserUuid(const std::string& sid) {
        if (sid.empty()) return std::nullopt;

        std::string key = makeKey(sid);
        auto val = redis_.hget(key, "user_uuid");
        if (!val) return std::nullopt;
        return *val;
    }

    void RedisSessionStore::destroySession(const std::string& sid) {
        if (sid.empty()) return;
        redis_.del(makeKey(sid));
    }

}
