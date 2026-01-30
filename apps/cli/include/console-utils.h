#pragma once

#include <cstddef>
#include <string>

#include "ui-common.h"

void setColor(ConsoleColor fg, ConsoleColor bg);
void resetColor();
void clearScreen();

InputAction getInput();
size_t utf8_len(const std::string& s);
std::string readPassword();
