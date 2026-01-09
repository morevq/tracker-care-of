#include "console-utils.h"

#include <conio.h>

#ifdef _WIN32
#include <windows.h>
#endif

static WORD toWinColor(ConsoleColor c) {
    return static_cast<WORD>(c);
}

// установка цвета
void setColor(ConsoleColor textColor, ConsoleColor bgColor) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(
        hConsole,
        (toWinColor(bgColor) << 4) | toWinColor(textColor)
    );
#endif
}

// сброс цвета
void resetColor() {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(
        hConsole,
        (toWinColor(ConsoleColor::Black) << 4) |
        toWinColor(ConsoleColor::White)
    );
#endif
}

// чтение клавиши
InputAction getInput() {
    int ch = _getch();

    if (ch == 0 || ch == 224) {
        ch = _getch();
        if (ch == 72) return InputAction::Up;
        if (ch == 80) return InputAction::Down;
        return InputAction::None;
    }

    if (ch == 'w' || ch == 'W') return InputAction::Up;
    if (ch == 's' || ch == 'S') return InputAction::Down;
    if (ch == 13) return InputAction::Enter;
    if (ch == 27) return InputAction::Escape;

    return InputAction::None;
}

// UTF-8 длина
size_t utf8_len(const std::string& s) {
    size_t count = 0;
    for (unsigned char c : s) {
        if ((c & 0xC0) != 0x80) ++count;
    }
    return count;
}
