#pragma once
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace tracker_api {

	struct CreatePatientRequest {
		std::string userUuid;
		std::string name;
		std::optional<std::string> birth_date;
	};

	struct PatientResponse {
		int id_patient;
		std::string user_uuid;
		std::string name;
		std::optional<std::string> birth_date;
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CreatePatientRequest, userUuid, name, birth_date)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PatientResponse, id_patient, user_uuid, name, birth_date)

};