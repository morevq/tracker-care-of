#include "api-client.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>

using json = nlohmann::json;

ApiClient::ApiClient(const std::string& baseUrl) : baseUrl(baseUrl) {}

std::string ApiClient::extractSessionCookie(const cpr::Response& r) {
    std::cerr << "DEBUG: Status code: " << r.status_code << '\n';
    
    auto cookies = r.cookies;
    std::cerr << "DEBUG: Checking CPR parsed cookies...\n";
    for (const auto& cookie : cookies) {
        std::cerr << "DEBUG: Cookie name: '" << cookie.GetName() << "', value: '" << cookie.GetValue() << "'\n";
        if (cookie.GetName() == "__Host-session") {
            std::cerr << "DEBUG: Found session cookie via CPR!\n";
            return cookie.GetName() + "=" + cookie.GetValue();
        }
    }
    
    std::cerr << "DEBUG: No session cookie found in CPR cookies list\n";

    auto findSessionCookieInHeader = [](const std::string& headerValue) {
        constexpr const char* kCookieName = "__Host-session=";
        auto pos = headerValue.find(kCookieName);
        if (pos == std::string::npos) {
            return std::string{};
        }
        pos += std::strlen(kCookieName);
        auto end = headerValue.find(';', pos);
        if (end == std::string::npos) {
            end = headerValue.size();
        }
        return headerValue.substr(pos, end - pos);
    };

    auto toLower = [](std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    };

    std::cerr << "DEBUG: Checking Set-Cookie headers...\n";
    for (const auto& headerEntry : r.header) {
        const auto& key = headerEntry.first;
        const auto& value = headerEntry.second;
        if (toLower(key) != "set-cookie") {
            continue;
        }
        auto sessionValue = findSessionCookieInHeader(value);
        if (!sessionValue.empty()) {
            std::cerr << "DEBUG: Found session cookie in Set-Cookie header!\n";
            return "__Host-session=" + sessionValue;
        }
    }

    if (!r.raw_header.empty()) {
        std::cerr << "DEBUG: Checking raw headers...\n";
        size_t start = 0;
        while (start < r.raw_header.size()) {
            auto end = r.raw_header.find('\n', start);
            if (end == std::string::npos) {
                end = r.raw_header.size();
            }
            auto line = r.raw_header.substr(start, end - start);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            auto lowerLine = toLower(line);
            constexpr const char* kHeaderPrefix = "set-cookie:";
            if (lowerLine.rfind(kHeaderPrefix, 0) == 0) {
                auto headerValue = line.substr(std::strlen(kHeaderPrefix));
                auto sessionValue = findSessionCookieInHeader(headerValue);
                if (!sessionValue.empty()) {
                    std::cerr << "DEBUG: Found session cookie in raw headers!\n";
                    return "__Host-session=" + sessionValue;
                }
            }

            if (end == r.raw_header.size()) {
                break;
            }
            start = end + 1;
        }
    }

    std::cerr << "DEBUG: Response text: " << r.text << '\n';
    return "";
}

bool ApiClient::registerUser(const std::string& email, const std::string& password) {
    json body = {
        {"email", email},
        {"password", password}
    };

    cpr::Response r = cpr::Post(
        cpr::Url{baseUrl + "/api/auth/register"},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{body.dump()}
    );

    if (r.status_code == 201) {
        std::string cookie = extractSessionCookie(r);
        if (!cookie.empty()) {
            sessionCookie = cookie;
            return true;
        }
    }
    return false;
}

bool ApiClient::loginUser(const std::string& email, const std::string& password) {
    json body = {
        {"email", email},
        {"password", password}
    };

    cpr::Response r = cpr::Post(
        cpr::Url{baseUrl + "/api/auth/login"},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{body.dump()}
    );

    if (r.status_code == 200) {
        std::string cookie = extractSessionCookie(r);
        if (!cookie.empty()) {
            sessionCookie = cookie;
            return true;
        }
    }
    else if (r.status_code == 0 || r.error) {
        std::cerr << "Network error: " << r.error.message << '\n';
        return false;
    }
    return false;
}

void ApiClient::logout() {
    cpr::Post(
        cpr::Url{baseUrl + "/api/auth/logout"},
        cpr::Header{{"Cookie", sessionCookie}}
    );
    sessionCookie.clear();
}

bool ApiClient::deleteUser() {
    cpr::Response r = cpr::Delete(
        cpr::Url{ baseUrl + "/api/auth/user" },
        cpr::Header{ {"Cookie", sessionCookie} }
    );

    if (r.status_code == 0 || r.error) {
        std::cerr << "Network error in deleteUser: " << r.error.message << '\n';
        return false;
    }

    if (r.status_code == 204) {
        sessionCookie.clear();
        return true;
    }
    return false;
}

std::vector<ApiClient::PatientDto> ApiClient::getPatients() {
cpr::Response r = cpr::Get(
    cpr::Url{baseUrl + "/api/patients"},
    cpr::Header{{"Cookie", sessionCookie}}
);

    std::vector<PatientDto> patients;
    
    if (r.status_code == 0 || r.error) {
        std::cerr << "Network error in getPatients: " << r.error.message << '\n';
        return patients;
    }
    
    if (r.status_code == 200) {
        try {
            auto j = json::parse(r.text);
            for (const auto& item : j) {
                patients.push_back({
                    item["id"],
                    item["name"],
                    item["birth_date"].is_null() ? std::nullopt : std::optional<std::string>(item["birth_date"]),
                    item["created_at"]
                });
            }
        } catch (const json::exception& e) {
            std::cerr << "JSON parsing error in getPatients: " << e.what() << '\n';
        }
    }
    return patients;
}

bool ApiClient::createPatient(const std::string& name, const std::optional<std::string>& birth_date) {
    json body = {
        {"name", name},
        {"birth_date", birth_date.has_value() ? json(*birth_date) : json(nullptr)}
    };

    cpr::Response r = cpr::Post(
        cpr::Url{baseUrl + "/api/patients"},
        cpr::Header{{"Content-Type", "application/json"}, {"Cookie", sessionCookie}},
        cpr::Body{body.dump()}
    );

    return r.status_code == 201;
}

std::optional<ApiClient::PatientDto> ApiClient::getPatientById(int id) {
cpr::Response r = cpr::Get(
    cpr::Url{baseUrl + "/api/patients/" + std::to_string(id)},
    cpr::Header{{"Cookie", sessionCookie}}
);

    if (r.status_code == 0 || r.error) {
        std::cerr << "Network error in getPatientById: " << r.error.message << '\n';
        return std::nullopt;
    }

    if (r.status_code == 200) {
        try {
            auto j = json::parse(r.text);
            return PatientDto{
                j["id"],
                j["name"],
                j["birth_date"].is_null() ? std::nullopt : std::optional<std::string>(j["birth_date"]),
                j["created_at"]
            };
        } catch (const json::exception& e) {
            std::cerr << "JSON parsing error in getPatientById: " << e.what() << '\n';
        }
    }
    return std::nullopt;
}

bool ApiClient::deletePatient(int id) {
cpr::Response r = cpr::Delete(
    cpr::Url{ baseUrl + "/api/patients/" + std::to_string(id) },
    cpr::Header{ {"Cookie", sessionCookie} }
);

    if (r.status_code == 0 || r.error) {
        std::cerr << "Network error in deletePatient: " << r.error.message << '\n';
        return false;
    }

    return r.status_code == 204;
}

std::vector<ApiClient::WaterDto> ApiClient::getWaterData() {
cpr::Response r = cpr::Get(
    cpr::Url{baseUrl + "/api/water"},
    cpr::Header{{"Cookie", sessionCookie}}
);

    std::vector<WaterDto> waterRecords;
    
    if (r.status_code == 0 || r.error) {
        std::cerr << "Network error in getWaterData: " << r.error.message << '\n';
        return waterRecords;
    }
    
    if (r.status_code == 200) {
        try {
            auto j = json::parse(r.text);
            for (const auto& item : j) {
                waterRecords.push_back({
                    item["id"],
                    item["lastWater"],
                    item["frequency"],
                    item["frequencyMeasure"]
                });
            }
        } catch (const json::exception& e) {
            std::cerr << "JSON parsing error in getWaterData: " << e.what() << '\n';
        }
    }
    return waterRecords;
}

bool ApiClient::deleteWater(int id) {
cpr::Response r = cpr::Delete(
    cpr::Url{ baseUrl + "/api/water/" + std::to_string(id) },
    cpr::Header{ {"Cookie", sessionCookie} }
);

    if (r.status_code == 0 || r.error) {
        std::cerr << "Network error in deleteWater: " << r.error.message << '\n';
        return false;
    }

    return r.status_code == 204;
}

std::vector<ApiClient::AnamnesisDto> ApiClient::getAnamnesisByPatient(int patientId) {
cpr::Response r = cpr::Get(
    cpr::Url{baseUrl + "/api/anamnesis/" + std::to_string(patientId)},
    cpr::Header{{"Cookie", sessionCookie}}
);

    std::vector<AnamnesisDto> anamnesisRecords;
    
    if (r.status_code == 0 || r.error) {
        std::cerr << "Network error in getAnamnesisByPatient: " << r.error.message << '\n';
        return anamnesisRecords;
    }
    
    if (r.status_code == 200) {
        try {
            auto j = json::parse(r.text);
            for (const auto& item : j) {
                    if (!item.contains("id") || !item.contains("description") || !item.contains("date")) {
                    std::cerr << "Warning: Skipping anamnesis record with missing fields\n";
                    continue;
                }
                
                anamnesisRecords.push_back({
                    item["id"],
                    item["description"],
                    item.contains("photo_url") && !item["photo_url"].is_null() 
                        ? std::optional<std::string>(item["photo_url"]) 
                        : std::nullopt,
                    item["date"]
                });
            }
        } catch (const json::exception& e) {
            std::cerr << "JSON parsing error in getAnamnesisByPatient: " << e.what() << '\n';
            std::cerr << "Response body: " << r.text << '\n';
        }
    }
    return anamnesisRecords;
}

bool ApiClient::createAnamnesis(int patientId, const std::string& description, const std::optional<std::string>& photo_url) {
    json body = {
        {"patient_id", patientId},
        {"description", description},
        {"photo_url", photo_url.has_value() ? json(*photo_url) : json(nullptr)}
    };

    cpr::Response r = cpr::Post(
        cpr::Url{baseUrl + "/api/anamnesis"},
        cpr::Header{{"Content-Type", "application/json"}, {"Cookie", sessionCookie}},
        cpr::Body{body.dump()}
    );

    if (r.status_code == 0 || r.error) {
        std::cerr << "Network error in createAnamnesis: " << r.error.message << '\n';
        return false;
    }

    if (r.status_code != 201) {
        std::cerr << "Failed to create anamnesis. Status code: " << r.status_code << '\n';
        std::cerr << "Server response: " << r.text << '\n';
        return false;
    }

    std::cout << "Anamnesis created successfully!\n";
    return true;
}

bool ApiClient::deleteAnamnesis(int id) {
    cpr::Response r = cpr::Delete(
        cpr::Url{ baseUrl + "/api/anamnesis/" + std::to_string(id) },
        cpr::Header{ {"Cookie", sessionCookie} }
    );

    if (r.status_code == 0 || r.error) {
        std::cerr << "Network error in deleteAnamnesis: " << r.error.message << '\n';
        return false;
    }

    return r.status_code == 204;
}