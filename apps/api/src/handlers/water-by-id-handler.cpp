#include "handlers/water-by-id-handler.hpp"

#include <stdexcept>

#include <nlohmann/json.hpp>

#include <userver/storages/postgres/component.hpp>

#include "../dto/water-dto.h"
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

    int id = 0;
    try {
        id = std::stoi(request.GetPathArg("id"));
    } catch (const std::exception&) {
        return http::BadRequest(request, "Invalid id");
    }

    using Method = userver::server::http::HttpMethod;
    switch (request.GetMethod()) {
        case Method::kGet:
            return HandleGet(request, id, *userUuid);
        case Method::kDelete:
            return HandleDelete(request, id, *userUuid);
        default:
            return http::SetStatus(
                request,
                userver::server::http::HttpStatus::kMethodNotAllowed,
                "Method not allowed");
    }
}

std::string WaterByIdHandler::HandleGet(
    const userver::server::http::HttpRequest& request, int patient_id,
    const std::string& user_uuid) const {
    try {
        auto patient = patient_repo_.getByID(patient_id);
        if (!patient) {
            return http::NotFound(request, "Patient not found");
        }
        if (patient->user_uuid != user_uuid) {
            return http::Forbidden(request);
        }

        auto water = water_repo_.getByPatientID(patient_id);
        if (!water) {
            return http::NotFound(request, "Water record not found");
        }

        WaterResponse response{water->idPatient, water->lastWater,
                               water->frequency, water->frequencyMeasure};

        request.GetHttpResponse().SetHeader(
            std::string_view{"Content-Type"}, std::string{"application/json"});
        return http::Ok(request, json(response).dump());
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

std::string WaterByIdHandler::HandleDelete(
    const userver::server::http::HttpRequest& request, int patient_id,
    const std::string& user_uuid) const {
    try {
        auto patient = patient_repo_.getByID(patient_id);
        if (!patient || patient->user_uuid != user_uuid) {
            return http::Forbidden(request);
        }

        water_repo_.deleteWater(patient_id);
        return http::NoContent(request);
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

}  // namespace tracker_api
