#include "handlers/water-list-handler.hpp"

#include <nlohmann/json.hpp>

#include <userver/storages/postgres/component.hpp>

#include "../dto/water-dto.h"
#include "components/session-store-component.hpp"
#include "handlers/http-helpers.hpp"

namespace tracker_api {

namespace {
using json = nlohmann::json;
}

WaterListHandler::WaterListHandler(
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

std::string WaterListHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    auto userUuid = GetCurrentUserUuid(request);
    if (!userUuid) {
        return http::Unauthorized(request);
    }

    using Method = userver::server::http::HttpMethod;
    switch (request.GetMethod()) {
        case Method::kGet:
            return HandleGet(request, *userUuid);
        case Method::kPost:
            return HandlePost(request, *userUuid);
        default:
            return http::SetStatus(
                request,
                userver::server::http::HttpStatus::kMethodNotAllowed,
                "Method not allowed");
    }
}

std::string WaterListHandler::HandleGet(
    const userver::server::http::HttpRequest& request,
    const std::string& user_uuid) const {
    try {
        auto records = water_repo_.getByUserUUID(user_uuid);

        std::vector<WaterResponse> response;
        response.reserve(records.size());
        for (const auto& w : records) {
            response.push_back(WaterResponse{w.idPatient, w.lastWater,
                                             w.frequency, w.frequencyMeasure});
        }

        request.GetHttpResponse().SetHeader(
            std::string_view{"Content-Type"}, std::string{"application/json"});
        return http::Ok(request, json(response).dump());
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

std::string WaterListHandler::HandlePost(
    const userver::server::http::HttpRequest& request,
    const std::string& user_uuid) const {
    try {
        auto body = json::parse(request.RequestBody());
        if (!body.contains("patient_id") || !body.contains("last_water")) {
            return http::BadRequest(
                request, "Missing required fields: patient_id and last_water");
        }

        int patientId = body["patient_id"].get<int>();
        std::string lastWater = body["last_water"].get<std::string>();

        auto patient = patient_repo_.getByID(patientId);
        if (!patient || patient->user_uuid != user_uuid) {
            return http::Forbidden(request);
        }

        if (!water_repo_.addWater(patientId, lastWater)) {
            return http::BadRequest(
                request,
                "Failed to add water record. Check that the date is not "
                "earlier than patient birth date.");
        }

        return http::Created(request, "Water record created successfully");
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

}  // namespace tracker_api
