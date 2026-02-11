#pragma once
#include <crow.h>
#include "tracker_db/repositories/water-repository.h"
#include "tracker_db/repositories/patient-repository.h"
#include "../dto/water-dto.h"

namespace tracker_api {

	class WaterController {
	private:
		WaterRepository& waterRepo;
		PatientRepository& patientRepo;

		crow::response getWaterData(const crow::request& req);
		crow::response getWaterByPatientId(const crow::request& req, int id);
		crow::response addWater(const crow::request& req);
		crow::response deleteWater(const crow::request& req, int id);

	public:
		WaterController(WaterRepository& waterRepo, PatientRepository& patientRepo);

		void registerRoutes(crow::SimpleApp& app);
	};

}