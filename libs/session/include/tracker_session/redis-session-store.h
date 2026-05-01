#pragma once
#include "session-store.h"

#include <memory>

#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/command_control.hpp>

namespace tracker_session {

    class RedisSessionStore final : public SessionStore {
    public:
        explicit RedisSessionStore(
            std::shared_ptr<userver::storages::redis::Client> client);

        std::string createSession(const std::string& userUuid, int ttlSeconds) override;
        std::optional<std::string> resolveUserUuid(const std::string& sid) override;
        void destroySession(const std::string& sid) override;
        void destroyAllSessionsForUser(const std::string& userUuid) override;

    private:
        static std::string makeSessionKey(const std::string& sid);
        static std::string makeUserKey(const std::string& userUuid);

        std::shared_ptr<userver::storages::redis::Client> client_;
    };

}
