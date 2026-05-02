#include "handlers/patient-anamnesis-handler.hpp"

#include <stdexcept>

#include <nlohmann/json.hpp>

#include <userver/storages/postgres/component.hpp>

#include "../dto/anamnesis-dto.h"
#include "components/session-store-component.hpp"
#include "handlers/http-helpers.hpp"

namespace tracker_api {

namespace {
using json = nlohmann::json;
}

PatientAnamnesisHandler::PatientAnamnesisHandler(
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

std::string PatientAnamnesisHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    auto userUuid = GetCurrentUserUuid(request);
    if (!userUuid) {
        return http::Unauthorized(request);
    }

    int patientId = 0;
    try {
        patientId = std::stoi(request.GetPathArg("patient_id"));
    } catch (const std::exception&) {
        return http::BadRequest(request, "Invalid patient_id");
    }

    using Method = userver::server::http::HttpMethod;
    switch (request.GetMethod()) {
        case Method::kGet:
            return HandleGet(request, patientId, *userUuid);
        case Method::kPost:
            return HandlePost(request, patientId, *userUuid);
        default:
            return http::SetStatus(
                request,
                userver::server::http::HttpStatus::kMethodNotAllowed,
                "Method not allowed");
    }
}

std::string PatientAnamnesisHandler::HandleGet(
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

        auto records = anamnesis_repo_.getByPatientId(patient_id);

        std::vector<AnamnesisResponse> response;
        response.reserve(records.size());
        for (const auto& a : records) {
            response.push_back(
                AnamnesisResponse{a.id, a.description, a.photo_url, a.created_at});
        }

        request.GetHttpResponse().SetHeader(
            std::string_view{"Content-Type"}, std::string{"application/json"});
        return http::Ok(request, json(response).dump());
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

std::string PatientAnamnesisHandler::HandlePost(
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

        auto data = json::parse(request.RequestBody());

        if (!data.contains("description") || !data["description"].is_string()) {
            return http::BadRequest(request, "description is required");
        }
        std::string description = data["description"].get<std::string>();

        std::optional<std::string> photoUrl;
        if (data.contains("photo_url") && !data["photo_url"].is_null()) {
            photoUrl = data["photo_url"].get<std::string>();
        }

        anamnesis_repo_.createAnamnesis(patient_id, description, photoUrl);
        return http::Created(request, "Anamnesis created successfully");
    } catch (const std::exception& e) {
        return http::BadRequest(request, e.what());
    }
}

}  // namespace tracker_api
