#include "handlers/patient-by-id-handler.hpp"

#include <stdexcept>

#include <nlohmann/json.hpp>

#include <userver/storages/postgres/component.hpp>

#include "../dto/patient-dto.h"
#include "components/session-store-component.hpp"
#include "handlers/http-helpers.hpp"

namespace tracker_api {

namespace {
using json = nlohmann::json;
}

PatientByIdHandler::PatientByIdHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : AuthenticatedHandlerBase(
          config, context,
          context.FindComponent<SessionStoreComponent>().GetStore()),
      patient_repo_(context
                        .FindComponent<userver::components::Postgres>(
                            "postgres-db")
                        .GetCluster()) {}

std::string PatientByIdHandler::HandleRequestThrow(
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
        case Method::kPatch:
            return HandlePatch(request, id, *userUuid);
        case Method::kDelete:
            return HandleDelete(request, id, *userUuid);
        default:
            return http::SetStatus(
                request,
                userver::server::http::HttpStatus::kMethodNotAllowed,
                "Method not allowed");
    }
}

std::string PatientByIdHandler::HandleGet(
    const userver::server::http::HttpRequest& request, int id,
    const std::string& user_uuid) const {
    try {
        auto patient = patient_repo_.getByID(id);
        if (!patient) {
            return http::NotFound(request, "Patient not found");
        }
        if (patient->user_uuid != user_uuid) {
            return http::Forbidden(request);
        }

        PatientResponse response{patient->id_patient, patient->name,
                                 patient->birth_date};

        request.GetHttpResponse().SetHeader(
            std::string_view{"Content-Type"}, std::string{"application/json"});
        return http::Ok(request, json(response).dump());
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

std::string PatientByIdHandler::HandlePatch(
    const userver::server::http::HttpRequest& request, int id,
    const std::string& user_uuid) const {
    try {
        auto patient = patient_repo_.getByID(id);
        if (!patient) {
            return http::NotFound(request, "Patient not found");
        }
        if (patient->user_uuid != user_uuid) {
            return http::Forbidden(request);
        }

        auto body = json::parse(request.RequestBody());

        std::optional<std::string> name;
        std::optional<std::string> birth_date;
        if (body.contains("name") && !body["name"].is_null()) {
            name = body["name"].get<std::string>();
        }
        if (body.contains("birth_date") && !body["birth_date"].is_null()) {
            birth_date = body["birth_date"].get<std::string>();
        }

        if (!name && !birth_date) {
            return http::BadRequest(request,
                                    "At least one field must be provided");
        }

        patient_repo_.updatePatient(id, name, birth_date);

        auto updated = patient_repo_.getByID(id);
        json response = {
            {"id_patient", updated->id_patient},
            {"name", updated->name},
        };
        if (updated->birth_date) {
            response["birth_date"] = *updated->birth_date;
        }

        request.GetHttpResponse().SetHeader(
            std::string_view{"Content-Type"}, std::string{"application/json"});
        return http::Ok(request, response.dump());
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

std::string PatientByIdHandler::HandleDelete(
    const userver::server::http::HttpRequest& request, int id,
    const std::string& user_uuid) const {
    try {
        auto patient = patient_repo_.getByID(id);
        if (!patient || patient->user_uuid != user_uuid) {
            return http::Forbidden(request);
        }

        patient_repo_.deletePatient(id);
        return http::NoContent(request);
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

}  // namespace tracker_api
