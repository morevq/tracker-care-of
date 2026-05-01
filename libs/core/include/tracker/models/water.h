#pragma once

#include <string>

struct Water {
    int idWater;
    int idPatient;
    std::string lastWater;
    int frequency;
    std::string frequencyMeasure;
};

struct WaterFrequency {
    int idPatient;
    int frequency;
    std::string measure;
};
