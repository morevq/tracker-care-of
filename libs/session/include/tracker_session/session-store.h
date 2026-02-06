#pragma once
#include <optional>
#include <string>

namespace tracker_session {

    class SessionStore {
    public:
        virtual ~SessionStore() = default;

        // returns session id (sid)
        virtual std::string createSession(const std::string& userUuid, int ttlSeconds) = 0;

        virtual std::optional<std::string> resolveUserUuid(const std::string& sid) = 0;

        virtual void destroySession(const std::string& sid) = 0;
    };

}
