#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <iostream>

inline std::string wideToUtf8(const std::wstring& wstr) {
	if (wstr.empty()) {
		return std::string();
	}

	const int sizeNeeded = WideCharToMultiByte(
		CP_UTF8,
		0,
		wstr.c_str(),
		static_cast<int>(wstr.size()),
		nullptr,
		0,
		nullptr,
		nullptr
	);

	std::string strTo(sizeNeeded, '\0');
	WideCharToMultiByte(
		CP_UTF8,
		0,
		wstr.c_str(),
		static_cast<int>(wstr.size()),
		strTo.data(),
		sizeNeeded,
		nullptr,
		nullptr
	);

	return strTo;
}

inline std::string readConsoleLineUtf8() {
	std::wstring wline;

	while (true) {
		const int ch = _getch();

		if (ch == 13) { // Enter
			std::cout << std::endl;
			break;
		}

		if (ch == 8) { // Backspace
			if (!wline.empty()) {
				wline.pop_back();
				std::cout << "\b \b";
			}
			continue;
		}

		if (ch == 0 || ch == 224) { // служебные клавиши (стрелки и т.п.)
			_getch();
			continue;
		}

		if (ch == 27) { // Esc
			wline.clear();
			std::cout << std::endl;
			break;
		}

		// Поддержка Unicode (UTF-16)
		wchar_t wch;
		if (ch >= 0 && ch <= 127) {
			// ASCII символ
			wch = static_cast<wchar_t>(ch);
		} else {
			// Многобайтовый символ - используем Windows API
			char mbChar[2] = { static_cast<char>(ch), 0 };
			MultiByteToWideChar(CP_ACP, 0, mbChar, 1, &wch, 1);
		}

		wline += wch;
		std::wcout << wch;
		std::wcout.flush();
	}

	return wideToUtf8(wline);
}

inline void initWinConsoleUnicode() {
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
}

#endif