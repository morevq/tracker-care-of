#pragma once

#include <memory>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#include <tracker_session/redis-session-store.h>
#include <tracker_session/session-store.h>

namespace tracker_api {

class SessionStoreComponent final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "session-store";

    SessionStoreComponent(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context);

    tracker_session::SessionStore& GetStore() const noexcept { return *store_; }

private:
    std::unique_ptr<tracker_session::RedisSessionStore> store_;
};

}  // namespace tracker_api
