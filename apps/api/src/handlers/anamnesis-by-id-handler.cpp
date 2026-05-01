#include "handlers/anamnesis-by-id-handler.hpp"

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

AnamnesisByIdHandler::AnamnesisByIdHandler(
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

std::string AnamnesisByIdHandler::HandleRequestThrow(
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

std::string AnamnesisByIdHandler::HandleGet(
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

std::string AnamnesisByIdHandler::HandlePatch(
    const userver::server::http::HttpRequest& request, int anamnesis_id,
    const std::string& user_uuid) const {
    try {
        auto anamnesis = anamnesis_repo_.getByID(anamnesis_id);
        if (!anamnesis) {
            return http::NotFound(request, "Anamnesis not found");
        }

        auto patient = patient_repo_.getByID(anamnesis->id_patient);
        if (!patient || patient->user_uuid != user_uuid) {
            return http::Forbidden(request);
        }

        auto body = json::parse(request.RequestBody());

        std::optional<std::string> description;
        std::optional<std::string> date;
        std::optional<std::string> photo_url;
        if (body.contains("description") && !body["description"].is_null()) {
            description = body["description"].get<std::string>();
        }
        if (body.contains("date") && !body["date"].is_null()) {
            date = body["date"].get<std::string>();
        }
        if (body.contains("photo_url") && !body["photo_url"].is_null()) {
            photo_url = body["photo_url"].get<std::string>();
        }

        if (!description && !date && !photo_url) {
            return http::BadRequest(request,
                                    "At least one field must be provided");
        }

        anamnesis_repo_.updateAnamnesis(anamnesis_id, description, date,
                                        photo_url);

        auto updated = anamnesis_repo_.getByID(anamnesis_id);
        json response = {
            {"id", updated->id},
            {"patient_id", updated->id_patient},
            {"description", updated->description},
            {"created_at", updated->created_at},
        };
        if (updated->photo_url) {
            response["photo_url"] = *updated->photo_url;
        }

        request.GetHttpResponse().SetHeader(
            std::string_view{"Content-Type"}, std::string{"application/json"});
        return http::Ok(request, response.dump());
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

std::string AnamnesisByIdHandler::HandleDelete(
    const userver::server::http::HttpRequest& request, int anamnesis_id,
    const std::string& /*user_uuid*/) const {
    try {
        anamnesis_repo_.deleteAnamnesis(anamnesis_id);
        return http::NoContent(request);
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

}  // namespace tracker_api
