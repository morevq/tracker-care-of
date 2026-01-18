#include "show-anamnesis-ui.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../repositories/anamnesis-repository.h"
#include "console-utils.h"

void showAnamnesisUI(
    int patientId,
    PGconn* conn,
    const std::string& patientName
) {
    int selected = 0;

    while (true) {
        AnamnesisRepository repo(conn);
        auto data = repo.getByPatientId(patientId);

        std::vector<AnamnesisTableRow> rows;
        rows.reserve(data.size());

        for (const auto& a : data) {
            rows.push_back(AnamnesisTableRow{
                a.id,
                a.date,
                a.description,
            });
        }

        size_t widthDate = 10;
        size_t widthDesc = 11;

        for (const auto& r : rows) {
            widthDate = std::max(widthDate, utf8_len(r.date));
            widthDesc = std::max(widthDesc, utf8_len(r.description));
        }

        const size_t gap = 2;

        auto pad = [&](const std::string& s, size_t w) {
            const size_t visible = utf8_len(s);
            const size_t spaces = (w > visible ? w - visible : 0);
            return s + std::string(spaces + gap, ' ');
        };

        system("cls");

        std::cout << "Anamnesis for: ";
        setColor(ConsoleColor::LightCyan, ConsoleColor::Black);
        std::cout << patientName;
        resetColor();
        std::cout << "\n\n";

        std::cout << "Use arrows / W S to navigate, Esc to go back\n\n";

        std::cout << std::left
                  << std::setw(static_cast<int>(widthDate + gap)) << "Date"
                  << std::setw(static_cast<int>(widthDesc + gap)) << "Description"
                  << '\n';

        std::cout << std::string(widthDate + widthDesc + gap * 2, '-') << '\n';

        if (rows.empty()) {
            setColor(ConsoleColor::Yellow, ConsoleColor::Black);
            std::cout << "No anamnesis records\n";
            resetColor();
        } else {
            for (size_t i = 0; i < rows.size(); ++i) {
                const auto& r = rows[i];

                if (static_cast<int>(i) == selected) {
                    setColor(ConsoleColor::LightCyan, ConsoleColor::Blue);
                    std::cout << pad(r.date, widthDate);

                    setColor(ConsoleColor::LightGreen, ConsoleColor::Blue);
                    std::cout << pad(r.description, widthDesc);

                    resetColor();
                } else {
                    setColor(ConsoleColor::Cyan, ConsoleColor::Black);
                    std::cout << pad(r.date, widthDate);

                    setColor(ConsoleColor::Green, ConsoleColor::Black);
                    std::cout << pad(r.description, widthDesc);

                    resetColor();
                }
                std::cout << '\n';
            }
        }

        const InputAction action = getInput();

        switch (action) {
            case InputAction::Up:
                if (!rows.empty()) {
                    selected = (selected + static_cast<int>(rows.size()) - 1) % static_cast<int>(rows.size());
                }
                break;

            case InputAction::Down:
                if (!rows.empty()) {
                    selected = (selected + 1) % static_cast<int>(rows.size());
                }
                break;

            case InputAction::Escape:
                return;

            default:
                break;
        }
    }
}
