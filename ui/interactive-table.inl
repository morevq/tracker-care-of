#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <conio.h>
#include <optional>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

// Цвета консоли
enum ConsoleColor {
    BLACK = 0, BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, LIGHT_GRAY,
    DARK_GRAY, LIGHT_BLUE, LIGHT_GREEN, LIGHT_CYAN, LIGHT_RED, LIGHT_MAGENTA, LIGHT_YELLOW, WHITE
};

// Установка цвета
inline void setColor(ConsoleColor textColor, ConsoleColor bgColor) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (bgColor << 4) | textColor);
#endif
}

// Сброс цвета
inline void resetColor() {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (BLACK << 4) | WHITE);
#endif
}

// Чтение клавиши
inline int getInput() {
    int ch = _getch();
    if (ch == 0 || ch == 224) {
        ch = _getch();
        if (ch == 72) return 'U'; // вверх
        if (ch == 80) return 'D'; // вниз
        return 0;
    }
    else if (ch == 'w' || ch == 'W') return 'U';
    else if (ch == 's' || ch == 'S') return 'D';
    else if (ch == 13) return 'E'; // Enter
    return 0;
}

// Структура строки таблицы
struct PatientTableRow {
    std::string name;
    std::string birthDate;
    std::string age;
    int id_patient;
};

// Считаем видимую длину UTF-8 строки
inline size_t utf8_len(const std::string& s) {
    size_t count = 0;
    for (unsigned char c : s) {
        if ((c & 0xC0) != 0x80) ++count;
    }
    return count;
}

// Интерфейс таблицы
inline int interactiveTable(const std::vector<PatientTableRow>& rows) {
    if (rows.empty()) return -1;

    int selected = 0;

    // Вычисляем ширину колонок по видимым символам
    size_t widthName = 4;
    size_t widthBirth = 10;
    size_t widthAge = 3;

    for (const auto& r : rows) {
        size_t ln = utf8_len(r.name);
        size_t lb = utf8_len(r.birthDate);
        size_t la = utf8_len(r.age);
        if (ln > widthName) widthName = ln;
        if (lb > widthBirth) widthBirth = lb;
        if (la > widthAge) widthAge = la;
    }

    const size_t gap = 2; // пробел между колонками

#ifdef _WIN32
    // Включаем поддержку ANSI кодов (Windows 10+)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    while (true) {
        system("cls");
        std::cout << "Use arrow keys or W/S to navigate, Enter to select\n\n";

        // Заголовок таблицы
        std::cout << std::left
            << std::setw(widthName + gap) << "Name"
            << std::setw(widthBirth + gap) << "Birth Date"
            << std::setw(widthAge + gap) << "Age"
            << std::endl;

        std::cout << std::string(widthName + widthBirth + widthAge + gap * 3, '-') << std::endl;

        for (size_t i = 0; i < rows.size(); ++i) {
            const auto& r = rows[i];

            // Подготавливаем паддинг для UTF-8 строк
            size_t padName = (widthName > utf8_len(r.name)) ? (widthName - utf8_len(r.name)) : 0;
            size_t padBirth = (widthBirth > utf8_len(r.birthDate)) ? (widthBirth - utf8_len(r.birthDate)) : 0;
            size_t padAge = (widthAge > utf8_len(r.age)) ? (widthAge - utf8_len(r.age)) : 0;

            // Формируем строки с паддингом
            std::string nameStr = r.name + std::string(padName + gap, ' ');
            std::string birthStr = r.birthDate + std::string(padBirth + gap, ' ');
            std::string ageStr = r.age + std::string(padAge + gap, ' ');

            if ((int)i == selected) {
                // Выделенная строка: фон синий, разные цвета текста для колонок
                setColor(LIGHT_CYAN, BLUE);   std::cout << nameStr;
                setColor(LIGHT_GREEN, BLUE);  std::cout << birthStr;
                setColor(LIGHT_YELLOW, BLUE); std::cout << ageStr << std::endl;
                resetColor();
            }
            else {
                // Обычная строка: фон черный, разные цвета для колонок
                setColor(CYAN, BLACK);   std::cout << nameStr;
                setColor(GREEN, BLACK);  std::cout << birthStr;
                setColor(YELLOW, BLACK); std::cout << ageStr << std::endl;
                resetColor();
            }
        }

        int input = getInput();
        if (input == 'U') selected = (selected - 1 + rows.size()) % rows.size();
        else if (input == 'D') selected = (selected + 1) % rows.size();
        else if (input == 'E') return rows[selected].id_patient;
    }
}
