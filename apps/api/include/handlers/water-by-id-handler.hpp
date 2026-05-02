#pragma once

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

#include <tracker_db/repositories/patient-repository.h>
#include <tracker_db/repositories/water-repository.h>

#include "handlers/authenticated-handler.hpp"

namespace tracker_api {

class WaterByIdHandler final : public AuthenticatedHandlerBase {
public:
    static constexpr std::string_view kName = "water-by-id-handler";

    WaterByIdHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

private:
    std::string HandleGet(const userver::server::http::HttpRequest& request,
                          int id_water,
                          const std::string& user_uuid) const;
    std::string HandleDelete(const userver::server::http::HttpRequest& request,
                             int id_water,
                             const std::string& user_uuid) const;

    mutable WaterRepository water_repo_;
    mutable PatientRepository patient_repo_;
};

}  // namespace tracker_api
