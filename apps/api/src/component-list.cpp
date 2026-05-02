#include "component-list.hpp"

#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include "components/session-store-component.hpp"
#include "handlers/anamnesis-by-id-handler.hpp"
#include "handlers/auth-handler.hpp"
#include "handlers/patient-anamnesis-handler.hpp"
#include "handlers/patient-by-id-handler.hpp"
#include "handlers/patient-list-handler.hpp"
#include "handlers/patient-water-handler.hpp"
#include "handlers/water-by-id-handler.hpp"
#include "handlers/water-frequency-handler.hpp"

userver::components::ComponentList MakeComponentList() {
    auto components = userver::components::MinimalServerComponentList();

    components
        .Append<userver::components::TestsuiteSupport>()
        .Append<userver::components::Secdist>()
        .Append<userver::components::DefaultSecdistProvider>()
        .Append<userver::clients::dns::Component>()
        .Append<userver::components::Postgres>("postgres-db")
        .Append<userver::components::Redis>("redis-sentinel")
        .Append<tracker_api::SessionStoreComponent>()
        .Append<tracker_api::AuthHandler>()
        .Append<tracker_api::PatientListHandler>()
        .Append<tracker_api::PatientByIdHandler>()
        .Append<tracker_api::PatientAnamnesisHandler>()
        .Append<tracker_api::AnamnesisByIdHandler>()
        .Append<tracker_api::PatientWaterHandler>()
        .Append<tracker_api::WaterByIdHandler>()
        .Append<tracker_api::WaterFrequencyHandler>();

    return components;
}
