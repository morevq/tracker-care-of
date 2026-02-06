#pragma once
#include "../tracker_session/session-store.h"

#include <memory>
#include <sw/redis++/redis++.h>

namespace tracker_session {

    class RedisSessionStore final : public SessionStore {
    public:
        explicit RedisSessionStore(const std::string& redisUri);

        std::string createSession(const std::string& userUuid, int ttlSeconds) override;
        std::optional<std::string> resolveUserUuid(const std::string& sid) override;
        void destroySession(const std::string& sid) override;

    private:
        static std::string makeKey(const std::string& sid);

        sw::redis::Redis redis_;
    };

}
