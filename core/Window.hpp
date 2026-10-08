#pragma once
#include <Windows.h>
#include <windowsx.h>
#include <hwndControl/HwndControl.hpp>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#include <shellscalingapi.h>
#pragma comment(lib, "Shcore.lib")
#include <nlohmann/json.hpp>
#include "../global.h"
#include "MessageBox.hpp"



inline static HWND CreateMainWindow(LPCWSTR CLASS_NAME, LPCWSTR APP_NAME, HINSTANCE hInstance, int widthDip, int heightDip) {
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

	auto DipToPixel = [](int dip, UINT dpi) {
		return MulDiv(dip, dpi, 96);
		};

	UINT dpi = GetDpiForSystem();
	int width = DipToPixel(widthDip, dpi);
	int height = DipToPixel(heightDip, dpi);

	HWND hwnd = CreateWindowExW(
		WS_EX_NOREDIRECTIONBITMAP,
		CLASS_NAME,
		APP_NAME,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	if (!hwnd)
		return nullptr;
	SetPropW(hwnd, L"MinWidth", reinterpret_cast<HANDLE>(static_cast<LONG_PTR>(width)));
	SetPropW(hwnd, L"MinHeight", reinterpret_cast<HANDLE>(static_cast<LONG_PTR>(height)));

	DWM_SYSTEMBACKDROP_TYPE backdropType = DWMSBT_TRANSIENTWINDOW;
	DwmSetWindowAttribute(
		hwnd,
		DWMWA_SYSTEMBACKDROP_TYPE,
		&backdropType,
		sizeof(backdropType)
	);
	DWM_WINDOW_CORNER_PREFERENCE cornerPreference = DWMWCP_ROUND;
	DwmSetWindowAttribute(
		hwnd,
		DWMWA_WINDOW_CORNER_PREFERENCE,
		&cornerPreference,
		sizeof(cornerPreference)
	);
	MARGINS margins = { 0, 0, 0, 0 };
	DwmExtendFrameIntoClientArea(hwnd, &margins);
	HwndControl::Init(hwnd);
	return hwnd;
}


constexpr UINT WM_WEBVIEW_RESPONSE = WM_APP + 10;
inline static WNDCLASSEXW CreateMainWNDCLASSEXW(LPCWSTR CLASS_NAME, HINSTANCE hInstance) {
	WNDCLASSEXW wc{};

	wc.cbSize = sizeof(wc);
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hbrBackground = nullptr;
	wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));

	wc.lpfnWndProc = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
		switch (uMsg) {
		case WM_CLOSE:
			if (AlertBox(L"确定退出程序？") == IDYES) {
				DestroyWindow(hwnd);
			}
			return 0;
		case WM_GETMINMAXINFO:
		{
			auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
			info->ptMinTrackSize.x = static_cast<LONG>(reinterpret_cast<LONG_PTR>(GetPropW(hwnd, L"MinWidth")));
			info->ptMinTrackSize.y = static_cast<LONG>(reinterpret_cast<LONG_PTR>(GetPropW(hwnd, L"MinHeight")));
			return 0;
		}
		case WM_SIZE:
			if (application) {
				application->Resize(LOWORD(lParam), HIWORD(lParam));
			}
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_WEBVIEW_RESPONSE:
			auto* res = reinterpret_cast<nlohmann::json*>(lParam);
			application->SendWebMessage(*res);
			delete res;
			return 0;
		}
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);

		};

	return wc;
}