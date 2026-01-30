#include "console-utils.h"

#include <conio.h>
#include <iostream>

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

// очистка экрана
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
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
	if (ch == '+') return InputAction::Add;
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

std::string readPassword() {
	std::string password;

#ifdef _WIN32
    while(true){
		const int ch = _getch();

        if (ch == 13) { // Enter
			std::cout << std::endl;
            break;
        }

        if (ch == 8) { // Backspace
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b"; // удаление символа из консоли
            }
            continue;
		}

		if (ch == 0 || ch == 224) { // служебные клавиши (стрелки и т.п.)
            _getch();
            continue;
        }

        if (ch == 27) { // Esc
            password.clear();
            std::cout << std::endl;
            break;
		}

        password += static_cast<char>(ch);
		std::cout << '*'; // отображение символа-звездочки
    }
#else
    termios oldt{};
    tcgetattr(STDIN_FILENO, &oldt);

    termios newt = oldt;
    newt.c_lflag &= static_cast<unsigned>(~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while (true) {
        char ch = '\0';
        const ssize_t n = ::read(STDIN_FILENO, &ch, 1);
        if (n <= 0) break;

        if (ch == '\n' || ch == '\r') {
            std::cout << '\n';
            break;
        }

        if (ch == 127 || ch == '\b') { // backspace
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b";
                std::cout.flush();
            }
            continue;
        }

        if (ch == 27) { // Esc
            std::cout << '\n';
            password.clear();
            break;
        }

        password.push_back(ch);
        std::cout << '*';
        std::cout.flush();
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    return password;
}