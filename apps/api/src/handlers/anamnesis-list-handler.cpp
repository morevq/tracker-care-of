#include "handlers/anamnesis-list-handler.hpp"

#include <nlohmann/json.hpp>

#include <userver/storages/postgres/component.hpp>

#include "components/session-store-component.hpp"
#include "handlers/http-helpers.hpp"

namespace tracker_api {

namespace {
using json = nlohmann::json;
}

AnamnesisListHandler::AnamnesisListHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : AuthenticatedHandlerBase(
          config, context,
          context.FindComponent<SessionStoreComponent>().GetStore()),
      anamnesis_repo_(context
                          .FindComponent<userver::components::Postgres>(
                              "postgres-db")
                          .GetCluster()),
      patient_repo_(context
                        .FindComponent<userver::components::Postgres>(
                            "postgres-db")
                        .GetCluster()) {}

std::string AnamnesisListHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    auto userUuid = GetCurrentUserUuid(request);
    if (!userUuid) {
        return http::Unauthorized(request);
    }

    if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
        return http::SetStatus(
            request, userver::server::http::HttpStatus::kMethodNotAllowed,
            "Method not allowed");
    }

    try {
        auto data = json::parse(request.RequestBody());

        int patientId = data.at("patient_id").get<int>();
        std::string description = data.at("description").get<std::string>();
        std::optional<std::string> photoUrl;
        if (data.contains("photo_url") && !data["photo_url"].is_null()) {
            photoUrl = data["photo_url"].get<std::string>();
        }

        auto patient = patient_repo_.getByID(patientId);
        if (!patient) {
            return http::NotFound(request, "Patient not found");
        }
        if (patient->user_uuid != *userUuid) {
            return http::Forbidden(request);
        }

        anamnesis_repo_.createAnamnesis(patientId, description, photoUrl);
        return http::Created(request, "Anamnesis created successfully");
    } catch (const std::exception& e) {
        return http::BadRequest(request, e.what());
    }
}

}  // namespace tracker_api
