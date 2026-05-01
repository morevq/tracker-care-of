#include "handlers/patient-water-handler.hpp"

#include <stdexcept>

#include <nlohmann/json.hpp>

#include <userver/storages/postgres/component.hpp>

#include "components/session-store-component.hpp"
#include "handlers/http-helpers.hpp"

namespace tracker_api {

namespace {
using json = nlohmann::json;

json WaterToJson(const Water& w) {
    return json{
        {"id_water", w.idWater},
        {"id_patient", w.idPatient},
        {"last_water", w.lastWater},
    };
}
}  // namespace

PatientWaterHandler::PatientWaterHandler(
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

std::string PatientWaterHandler::HandleRequestThrow(
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

std::string PatientWaterHandler::HandleGet(
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

        auto records = water_repo_.listByPatientID(patient_id);

        json response = json::array();
        for (const auto& w : records) {
            response.push_back(WaterToJson(w));
        }

        request.GetHttpResponse().SetHeader(
            std::string_view{"Content-Type"}, std::string{"application/json"});
        return http::Ok(request, response.dump());
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

std::string PatientWaterHandler::HandlePost(
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

        auto body = json::parse(request.RequestBody());
        if (!body.contains("last_water") || !body["last_water"].is_string()) {
            return http::BadRequest(request, "last_water is required");
        }
        std::string lastWater = body["last_water"].get<std::string>();

        auto idWater = water_repo_.addWater(patient_id, lastWater);
        if (!idWater) {
            return http::BadRequest(
                request,
                "Failed to add water record. Check that the date is not "
                "earlier than patient birth date.");
        }

        json response = {
            {"id_water", *idWater},
            {"id_patient", patient_id},
            {"last_water", lastWater},
        };
        request.GetHttpResponse().SetHeader(
            std::string_view{"Content-Type"}, std::string{"application/json"});
        return http::Created(request, response.dump());
    } catch (const std::exception& e) {
        return http::BadRequest(request, e.what());
    }
}

}  // namespace tracker_api
