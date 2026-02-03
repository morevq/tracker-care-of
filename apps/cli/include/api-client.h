#pragma once

#include <string>
#include <vector>
#include <optional>

struct ApiClient {
    std::string baseUrl;
    std::string sessionCookie;

    ApiClient(const std::string& baseUrl);

    bool registerUser(const std::string& email, const std::string& password);
    bool loginUser(const std::string& email, const std::string& password);
    void logout();

    struct PatientDto {
        int id;
        std::string name;
        std::optional<std::string> birth_date;
        std::string created_at;
    };
    std::vector<PatientDto> getPatients();
    bool createPatient(const std::string& name, const std::optional<std::string>& birth_date);
    std::optional<PatientDto> getPatientById(int id);
    bool deletePatient(int id);

    struct WaterDto {
        int id;
        std::string lastWater;
        int frequency;
        std::string frequencyMeasure;
    };
    std::vector<WaterDto> getWaterData();

    struct AnamnesisDto {
        int id;
        std::string description;
        std::optional<std::string> photo_url;
        std::string date;
    };
    std::vector<AnamnesisDto> getAnamnesisByPatient(int patientId);
    bool createAnamnesis(int patientId, const std::string& description, const std::optional<std::string>& photo_url);
};