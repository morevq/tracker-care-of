#include "handlers/water-by-id-handler.hpp"

#include <stdexcept>

#include <nlohmann/json.hpp>

#include <userver/storages/postgres/component.hpp>

#include "components/session-store-component.hpp"
#include "handlers/http-helpers.hpp"

namespace tracker_api {

namespace {
using json = nlohmann::json;
}

WaterByIdHandler::WaterByIdHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : AuthenticatedHandlerBase(
          config, context,
          context.FindComponent<SessionStoreComponent>().GetStore()),
      water_repo_(context
                      .FindComponent<userver::components::Postgres>(
                          "postgres-db")
                      .GetCluster()),
      patient_repo_(context
                        .FindComponent<userver::components::Postgres>(
                            "postgres-db")
                        .GetCluster()) {}

std::string WaterByIdHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    auto userUuid = GetCurrentUserUuid(request);
    if (!userUuid) {
        return http::Unauthorized(request);
    }

    int idWater = 0;
    try {
        idWater = std::stoi(request.GetPathArg("id"));
    } catch (const std::exception&) {
        return http::BadRequest(request, "Invalid id");
    }

    using Method = userver::server::http::HttpMethod;
    switch (request.GetMethod()) {
        case Method::kGet:
            return HandleGet(request, idWater, *userUuid);
        case Method::kDelete:
            return HandleDelete(request, idWater, *userUuid);
        default:
            return http::SetStatus(
                request,
                userver::server::http::HttpStatus::kMethodNotAllowed,
                "Method not allowed");
    }
}

std::string WaterByIdHandler::HandleGet(
    const userver::server::http::HttpRequest& request, int id_water,
    const std::string& user_uuid) const {
    try {
        auto water = water_repo_.getByID(id_water);
        if (!water) {
            return http::NotFound(request, "Water record not found");
        }

        auto patient = patient_repo_.getByID(water->idPatient);
        if (!patient || patient->user_uuid != user_uuid) {
            return http::Forbidden(request);
        }

        json response = {
            {"id_water", water->idWater},
            {"id_patient", water->idPatient},
            {"last_water", water->lastWater},
        };
        request.GetHttpResponse().SetHeader(
            std::string_view{"Content-Type"}, std::string{"application/json"});
        return http::Ok(request, response.dump());
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

std::string WaterByIdHandler::HandleDelete(
    const userver::server::http::HttpRequest& request, int id_water,
    const std::string& user_uuid) const {
    try {
        auto water = water_repo_.getByID(id_water);
        if (!water) {
            return http::NoContent(request);
        }

        auto patient = patient_repo_.getByID(water->idPatient);
        if (!patient || patient->user_uuid != user_uuid) {
            return http::Forbidden(request);
        }

        water_repo_.deleteByID(id_water);
        return http::NoContent(request);
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

}  // namespace tracker_api
