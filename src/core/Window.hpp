#pragma once
#include <Windows.h>
#include "../../resources/resource.h"

inline HWND CreateMainWindow(LPCWSTR CLASS_NAME, LPCWSTR APP_NAME, HINSTANCE hInstance, int widthDip, int heightDip) {
	auto DipToPixel = [](int dip, UINT dpi) {
		return MulDiv(dip, dpi, 96);
		};

	UINT dpi = GetDpiForSystem();
	int width = DipToPixel(widthDip, dpi);
	int height = DipToPixel(heightDip, dpi);

	return CreateWindowExW(
		0,
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
}

inline WNDCLASSEXW CreateMainWNDCLASSEXW(LPCWSTR CLASS_NAME, HINSTANCE hInstance) {
	WNDCLASSEXW wc{};

	wc.cbSize = sizeof(wc);
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));
	wc.hIcon = static_cast<HICON>(LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 256, 256, LR_DEFAULTCOLOR));
	wc.hIconSm = static_cast<HICON>(LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));

	wc.lpfnWndProc = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
		switch (uMsg) {
		case WM_CLOSE:
			DestroyWindow(hwnd);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		};

	return wc;
}