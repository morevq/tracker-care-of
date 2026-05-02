#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cpr/cpr.h>

struct ApiClient {
    std::string baseUrl;
    std::string sessionCookie;

    ApiClient(const std::string& baseUrl);

    bool registerUser(const std::string& email, const std::string& password);
    bool loginUser(const std::string& email, const std::string& password);
    void logout();
    bool deleteUser();

    struct PatientDto {
        int id;
        std::string name;
        std::optional<std::string> birth_date;
    };
    std::vector<PatientDto> getPatients();
    bool createPatient(const std::string& name, const std::optional<std::string>& birth_date);
    std::optional<PatientDto> getPatientById(int id);
    bool deletePatient(int id);

    struct WaterDto {
        int id_water;
        int id_patient;
        std::string lastWater;
    };
    std::vector<WaterDto> getWaterForPatient(int patientId);
    bool deleteWater(int id_water);

    struct WaterFrequencyDto {
        int frequency;
        std::string measure;
    };
    std::optional<WaterFrequencyDto> getWaterFrequency(int patientId);

    struct AnamnesisDto {
        int id;
        std::string description;
        std::optional<std::string> photo_url;
        std::string date;
    };
    std::vector<AnamnesisDto> getAnamnesisByPatient(int patientId);
    bool createAnamnesis(int patientId, const std::string& description, const std::optional<std::string>& photo_url);
    bool deleteAnamnesis(int id);

private:
    std::string extractSessionCookie(const cpr::Response& r);
};
