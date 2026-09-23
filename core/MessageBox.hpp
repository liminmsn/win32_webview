#pragma once
#include <Windows.h>
#include "../global.h"

inline static std::wstring StringToWString(const std::string& str, UINT codePage = CP_ACP) {
	if (str.empty()) return L"";
	int targetLen = MultiByteToWideChar(codePage, 0, str.c_str(), (int)str.length(), NULL, 0);
	if (targetLen <= 0) return L"";
	std::wstring wstr(targetLen, 0);
	MultiByteToWideChar(codePage, 0, str.c_str(), (int)str.length(), &wstr[0], targetLen);
	return wstr;
}

inline static int AlertInfo(LPCWSTR lpCaption) {
	return MessageBoxW(
		application->GetHwnd(),
		lpCaption,
		APP_NAME,
		MB_OK | MB_ICONEXCLAMATION
	);
}

inline static int AlertBox(LPCWSTR lpCaption) {
	return MessageBoxW(
		application->GetHwnd(),
		lpCaption,
		APP_NAME,
		MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2
	);
}