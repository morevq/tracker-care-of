#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>

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
	std::getline(std::wcin, wline);
	return wideToUtf8(wline);
}

inline void initWinConsoleUnicode() {
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
}

#endif