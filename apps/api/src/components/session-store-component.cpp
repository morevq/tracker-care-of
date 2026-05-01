#include "components/session-store-component.hpp"

#include <userver/storages/redis/component.hpp>

namespace tracker_api {

SessionStoreComponent::SessionStoreComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      store_(std::make_unique<tracker_session::RedisSessionStore>(
          context.FindComponent<userver::components::Redis>("redis-sentinel")
              .GetClient("tracker-sessions"))) {}

}  // namespace tracker_api
