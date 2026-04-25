#include "handlers/patient-list-handler.hpp"

#include <nlohmann/json.hpp>

#include <userver/storages/postgres/component.hpp>

#include "../dto/patient-dto.h"
#include "components/session-store-component.hpp"
#include "handlers/http-helpers.hpp"

namespace tracker_api {

namespace {
using json = nlohmann::json;
}

PatientListHandler::PatientListHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : AuthenticatedHandlerBase(
          config, context,
          context.FindComponent<SessionStoreComponent>().GetStore()),
      patient_repo_(context
                        .FindComponent<userver::components::Postgres>(
                            "postgres-db")
                        .GetCluster()) {}

std::string PatientListHandler::HandleRequestThrow(
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

std::string PatientListHandler::HandleGet(
    const userver::server::http::HttpRequest& request,
    const std::string& user_uuid) const {
    try {
        auto patients = patient_repo_.getByUserUUID(user_uuid);

        std::vector<PatientResponse> response;
        response.reserve(patients.size());
        for (const auto& p : patients) {
            response.push_back(
                PatientResponse{p.id_patient, p.name, p.birth_date});
        }

        request.GetHttpResponse().SetHeader(
            std::string_view{"Content-Type"}, std::string{"application/json"});
        return http::Ok(request, json(response).dump());
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

std::string PatientListHandler::HandlePost(
    const userver::server::http::HttpRequest& request,
    const std::string& user_uuid) const {
    try {
        auto data = json::parse(request.RequestBody());
        auto req = data.get<CreatePatientRequest>();

        patient_repo_.createPatient(user_uuid, req.name, req.birth_date);
        return http::Created(request, "Patient created successfully");
    } catch (const std::exception& e) {
        return http::BadRequest(request, e.what());
    }
}

}  // namespace tracker_api
