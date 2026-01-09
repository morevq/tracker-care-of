#pragma once

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "console-utils.h"
#include "interactive-table.h"

static void drawHeader(size_t widthName, size_t widthBirth, size_t widthAge, size_t gap) {
    setColor(ConsoleColor::LightCyan, ConsoleColor::Blue);
    std::cout << std::left << std::setw(static_cast<int>(widthName + gap)) << "Name";

    setColor(ConsoleColor::LightGreen, ConsoleColor::Blue);
    std::cout << std::left << std::setw(static_cast<int>(widthBirth + gap)) << "Birth Date";

    setColor(ConsoleColor::Yellow, ConsoleColor::Blue);
    std::cout << std::left << std::setw(static_cast<int>(widthAge + gap)) << "Age";

    resetColor();
    std::cout << '\n';
}

inline int interactiveTable(const std::vector<PatientTableRow>& rows) {
    if (rows.empty()) return -1;

    int selected = 0;

    size_t widthName = 4;
    size_t widthBirth = 10;
    size_t widthAge = 3;

    for (const auto& r : rows) {
        widthName = std::max(widthName, utf8_len(r.name));
        widthBirth = std::max(widthBirth, utf8_len(r.birth_date));
        widthAge = std::max(widthAge, utf8_len(r.age));
    }

    const size_t gap = 2;

    auto pad = [&](const std::string& s, size_t w) {
        const size_t visible = utf8_len(s);
        return s + std::string((w > visible ? w - visible : 0) + gap, ' ');
    };

    while (true) {
        system("cls");

        std::cout << "Use arrow keys or W/S to navigate, Enter to select, Esc to exit\n\n";

        drawHeader(widthName, widthBirth, widthAge, gap);

        std::cout << std::string(widthName + widthBirth + widthAge + gap * 3, '-') << '\n';

        for (size_t i = 0; i < rows.size(); ++i) {
            const auto& r = rows[i];

            const std::string nameStr = pad(r.name, widthName);
            const std::string birthStr = pad(r.birth_date, widthBirth);
            const std::string ageStr = pad(r.age, widthAge);

            if (static_cast<int>(i) == selected) {
                setColor(ConsoleColor::LightCyan, ConsoleColor::Blue);
                std::cout << nameStr;

                setColor(ConsoleColor::LightGreen, ConsoleColor::Blue);
                std::cout << birthStr;

                setColor(ConsoleColor::Yellow, ConsoleColor::Blue);
                std::cout << ageStr << '\n';
                resetColor();
            } else {
                setColor(ConsoleColor::Cyan, ConsoleColor::Black);
                std::cout << nameStr;

                setColor(ConsoleColor::Green, ConsoleColor::Black);
                std::cout << birthStr;

                setColor(ConsoleColor::Yellow, ConsoleColor::Black);
                std::cout << ageStr << '\n';
                resetColor();
            }
        }

        const InputAction action = getInput();

        switch (action) {
            case InputAction::Up:
                selected = (selected + static_cast<int>(rows.size()) - 1) % static_cast<int>(rows.size());
                break;

            case InputAction::Down:
                selected = (selected + 1) % static_cast<int>(rows.size());
                break;

            case InputAction::Enter:
                return rows[static_cast<size_t>(selected)].id_patient;

            case InputAction::Escape:
                return -1;

            default:
                break;
        }
    }
}
