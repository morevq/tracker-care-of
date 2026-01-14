#pragma once

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "console-utils.h"
#include "interactive-table.h"

static std::string waterDateOnly(const std::string& ts) {
    if (ts.empty()) return "-";
    if (ts.size() >= 10) return ts.substr(0, 10);
    return ts;
}

static std::string timesWordRu(int n) {
    const int mod100 = n % 100;
    if (mod100 >= 11 && mod100 <= 14) return "times";

    switch (n % 10) {
        case 1: return "time";
        case 2:
        case 3:
        case 4: return "times";
        default: return "times";
    }

    return "times";
}

static std::string waterFreqText(const Water& w) {
    if (w.frequency <= 0) return "-";
    if (w.frequencyMeasure.empty()) return std::to_string(w.frequency) + " " + timesWordRu(w.frequency);

    return std::to_string(w.frequency) + " " + timesWordRu(w.frequency) + " " + w.frequencyMeasure;
}

static void drawHeader(
    size_t widthName,
    size_t widthBirth,
    size_t widthAge,
    size_t widthWaterDate,
    size_t widthWaterFreq,
    size_t gap
) {
    setColor(ConsoleColor::LightCyan, ConsoleColor::Blue);
    std::cout << std::left << std::setw(static_cast<int>(widthName + gap)) << "Name";

    setColor(ConsoleColor::LightGreen, ConsoleColor::Blue);
    std::cout << std::left << std::setw(static_cast<int>(widthBirth + gap)) << "Birth Date";

    setColor(ConsoleColor::Yellow, ConsoleColor::Blue);
    std::cout << std::left << std::setw(static_cast<int>(widthAge + gap)) << "Age";

    setColor(ConsoleColor::LightBlue, ConsoleColor::Blue);
    std::cout << std::left << std::setw(static_cast<int>(widthWaterDate + gap)) << "Last water";

    setColor(ConsoleColor::Magenta, ConsoleColor::Blue);
    std::cout << std::left << std::setw(static_cast<int>(widthWaterFreq + gap)) << "Freq";

    resetColor();
    std::cout << '\n';
}

inline int interactiveTable(const std::vector<PatientTableRow>& rows) {
    if (rows.empty()) return -1;

    size_t selected = 0;

    size_t widthName = 4;
    size_t widthBirth = 10;
    size_t widthAge = 3;
    size_t widthWaterDate = 9;
    size_t widthWaterFreq = 4;

    for (const auto& r : rows) {
        const std::string lastWaterDate = waterDateOnly(r.water.lastWater);
        const std::string freqText = waterFreqText(r.water);

        widthName = std::max(widthName, utf8_len(r.name));
        widthBirth = std::max(widthBirth, utf8_len(r.birth_date));
        widthAge = std::max(widthAge, utf8_len(r.age));
        widthWaterDate = std::max(widthWaterDate, utf8_len(lastWaterDate));
        widthWaterFreq = std::max(widthWaterFreq, utf8_len(freqText));
    }

    const size_t gap = 2;

    auto pad = [&](const std::string& s, size_t w) {
        const size_t visible = utf8_len(s);
        return s + std::string((w > visible ? w - visible : 0) + gap, ' ');
    };

    while (true) {
        system("cls");

        std::cout << "Use arrow keys or W/S to navigate, Enter to select, Esc to exit\n\n";

        drawHeader(widthName, widthBirth, widthAge, widthWaterDate, widthWaterFreq, gap);

        std::cout << std::string(
            widthName + widthBirth + widthAge + widthWaterDate + widthWaterFreq + gap * 5,
            '-'
        ) << '\n';

        for (size_t i = 0; i < rows.size(); ++i) {
            const auto& r = rows[i];

            const std::string lastWaterDate = waterDateOnly(r.water.lastWater);
            const std::string freqText = waterFreqText(r.water);

            const std::string nameStr = pad(r.name, widthName);
            const std::string birthStr = pad(r.birth_date, widthBirth);
            const std::string ageStr = pad(r.age, widthAge);
            const std::string waterDateStr = pad(lastWaterDate, widthWaterDate);
            const std::string waterFreqStr = pad(freqText, widthWaterFreq);

            const bool isSelected = i == selected;

            if (isSelected) {
                setColor(ConsoleColor::LightCyan, ConsoleColor::Blue);
                std::cout << nameStr;

                setColor(ConsoleColor::LightGreen, ConsoleColor::Blue);
                std::cout << birthStr;

                setColor(ConsoleColor::Yellow, ConsoleColor::Blue);
                std::cout << ageStr;

                setColor(ConsoleColor::LightBlue, ConsoleColor::Blue);
                std::cout << waterDateStr;

                setColor(ConsoleColor::Magenta, ConsoleColor::Blue);
                std::cout << waterFreqStr << '\n';

                resetColor();
            } else {
                setColor(ConsoleColor::Cyan, ConsoleColor::Black);
                std::cout << nameStr;

                setColor(ConsoleColor::Green, ConsoleColor::Black);
                std::cout << birthStr;

                setColor(ConsoleColor::Yellow, ConsoleColor::Black);
                std::cout << ageStr;

                setColor(ConsoleColor::LightBlue, ConsoleColor::Black);
                std::cout << waterDateStr;

                setColor(ConsoleColor::Magenta, ConsoleColor::Black);
                std::cout << waterFreqStr << '\n';

                resetColor();
            }
        }

        const InputAction action = getInput();

        switch (action) {
            case InputAction::Up:
                if (selected == 0) {
                    selected = rows.size() - 1;
                } else {
                    --selected;
                }
                break;

            case InputAction::Down:
                selected = (selected + 1) % rows.size();
                break;

            case InputAction::Enter:
                return rows[selected].id_patient;

            case InputAction::Escape:
                return -1;

            default:
                break;
        }
    }
}
