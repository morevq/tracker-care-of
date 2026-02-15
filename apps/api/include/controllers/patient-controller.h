#pragma once
#include <crow.h>
#include "tracker_db/repositories/patient-repository.h"
#include "../dto/patient-dto.h"

namespace tracker_api {
	class PatientController {
	private:
		PatientRepository& patientRepo;
		crow::response createPatient(const crow::request& req);
		crow::response getPatients(const crow::request& req);
		crow::response getPatientById(const crow::request& req, int id);
		crow::response updatePatient(const crow::request& req, int id);
		crow::response deletePatient(const crow::request& req, int id);
	public:
		PatientController(PatientRepository& patientRepo);
		crow::Blueprint getBlueprint();
	};
}