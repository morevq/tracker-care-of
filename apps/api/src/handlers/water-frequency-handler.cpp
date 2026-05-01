#include "handlers/water-frequency-handler.hpp"

#include <stdexcept>
#include <string>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include <userver/logging/log.hpp>
#include <userver/storages/postgres/component.hpp>

#include "components/session-store-component.hpp"
#include "handlers/http-helpers.hpp"

namespace tracker_api {

namespace {
using json = nlohmann::json;

const std::unordered_set<std::string>& AllowedMeasures() {
    static const std::unordered_set<std::string> kAllowed{"days", "weeks", "hours"};
    return kAllowed;
}
}  // namespace

WaterFrequencyHandler::WaterFrequencyHandler(
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

bool WaterFrequencyHandler::EnsurePatientOwnedBy(
    const userver::server::http::HttpRequest& request, int patient_id,
    const std::string& user_uuid, std::string& out_response) const {
    auto patient = patient_repo_.getByID(patient_id);
    if (!patient) {
        out_response = http::NotFound(request, "Patient not found");
        return false;
    }
    if (patient->user_uuid != user_uuid) {
        out_response = http::Forbidden(request);
        return false;
    }
    return true;
}

std::string WaterFrequencyHandler::HandleRequestThrow(
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

    std::string ownership_response;
    if (!EnsurePatientOwnedBy(request, patientId, *userUuid, ownership_response)) {
        return ownership_response;
    }

    using Method = userver::server::http::HttpMethod;
    switch (request.GetMethod()) {
        case Method::kGet:
            return HandleGet(request, patientId);
        case Method::kPut:
            return HandlePut(request, patientId);
        case Method::kDelete:
            return HandleDelete(request, patientId);
        default:
            return http::SetStatus(
                request,
                userver::server::http::HttpStatus::kMethodNotAllowed,
                "Method not allowed");
    }
}

std::string WaterFrequencyHandler::HandleGet(
    const userver::server::http::HttpRequest& request, int patient_id) const {
    try {
        auto wf = water_repo_.getFrequency(patient_id);
        if (!wf) {
            return http::NotFound(request, "Water frequency not set");
        }

        json response = {
            {"frequency", wf->frequency},
            {"frequency_measure", wf->measure},
        };
        request.GetHttpResponse().SetHeader(
            std::string_view{"Content-Type"}, std::string{"application/json"});
        return http::Ok(request, response.dump());
    } catch (const std::exception& e) {
        LOG_ERROR() << "water-frequency GET failed for patient " << patient_id
                    << ": " << e.what();
        return http::InternalError(request, e.what());
    }
}

std::string WaterFrequencyHandler::HandlePut(
    const userver::server::http::HttpRequest& request, int patient_id) const {
    try {
        auto body = json::parse(request.RequestBody());

        if (!body.contains("frequency") || !body["frequency"].is_number_integer()) {
            return http::BadRequest(request,
                                    "frequency (positive integer) is required");
        }
        if (!body.contains("frequency_measure") ||
            !body["frequency_measure"].is_string()) {
            return http::BadRequest(request,
                                    "frequency_measure is required");
        }

        int frequency = body["frequency"].get<int>();
        std::string measure = body["frequency_measure"].get<std::string>();

        if (frequency <= 0) {
            return http::BadRequest(request, "frequency must be > 0");
        }
        if (!AllowedMeasures().count(measure)) {
            return http::BadRequest(
                request,
                "frequency_measure must be one of: days, weeks, hours");
        }

        try {
            water_repo_.upsertFrequency(patient_id, frequency, measure);
        } catch (const std::exception& e) {
            LOG_ERROR() << "water-frequency PUT (upsert) failed for patient "
                        << patient_id << ": " << e.what();
            return http::InternalError(request, e.what());
        }

        json response = {
            {"frequency", frequency},
            {"frequency_measure", measure},
        };
        request.GetHttpResponse().SetHeader(
            std::string_view{"Content-Type"}, std::string{"application/json"});
        return http::Ok(request, response.dump());
    } catch (const std::exception& e) {
        return http::BadRequest(request, e.what());
    }
}

std::string WaterFrequencyHandler::HandleDelete(
    const userver::server::http::HttpRequest& request, int patient_id) const {
    try {
        water_repo_.deleteFrequency(patient_id);
        return http::NoContent(request);
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

}  // namespace tracker_api
