#include "handlers/anamnesis-by-id-handler.hpp"

#include <stdexcept>

#include <nlohmann/json.hpp>

#include <userver/storages/postgres/component.hpp>

#include "components/session-store-component.hpp"
#include "handlers/http-helpers.hpp"

namespace tracker_api {

namespace {
using json = nlohmann::json;

json AnamnesisToJson(const Anamnesis& a) {
    json j = {
        {"id", a.id},
        {"id_patient", a.id_patient},
        {"description", a.description},
        {"date", a.created_at},
    };
    if (a.photo_url) {
        j["photo_url"] = *a.photo_url;
    } else {
        j["photo_url"] = nullptr;
    }
    return j;
}
}  // namespace

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

        request.GetHttpResponse().SetHeader(
            std::string_view{"Content-Type"}, std::string{"application/json"});
        return http::Ok(request, AnamnesisToJson(*anamnesis).dump());
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
        request.GetHttpResponse().SetHeader(
            std::string_view{"Content-Type"}, std::string{"application/json"});
        return http::Ok(request, AnamnesisToJson(*updated).dump());
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

std::string AnamnesisByIdHandler::HandleDelete(
    const userver::server::http::HttpRequest& request, int anamnesis_id,
    const std::string& user_uuid) const {
    try {
        auto anamnesis = anamnesis_repo_.getByID(anamnesis_id);
        if (!anamnesis) {
            return http::NoContent(request);
        }

        auto patient = patient_repo_.getByID(anamnesis->id_patient);
        if (!patient || patient->user_uuid != user_uuid) {
            return http::Forbidden(request);
        }

        anamnesis_repo_.deleteAnamnesis(anamnesis_id);
        return http::NoContent(request);
    } catch (const std::exception& e) {
        return http::InternalError(request, e.what());
    }
}

}  // namespace tracker_api
