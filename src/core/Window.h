#pragma once
#include <Windows.h>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#include <shellscalingapi.h>
#pragma comment(lib, "Shcore.lib")

inline HWND CreateMainWindow(LPCWSTR CLASS_NAME, LPCWSTR APP_NAME, HINSTANCE hInstance, int widthDip, int heightDip) {
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

	DWM_SYSTEMBACKDROP_TYPE backdropType = DWMSBT_TRANSIENTWINDOW;
	DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropType, sizeof(backdropType));

	DWM_WINDOW_CORNER_PREFERENCE cornerPreference = DWMWCP_ROUND;
	DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));

	MARGINS margins = { -1 };
	DwmExtendFrameIntoClientArea(hwnd, &margins);

	return hwnd;
}

inline WNDCLASSEXW CreateMainWNDCLASSEXW(LPCWSTR CLASS_NAME, HINSTANCE hInstance) {
	WNDCLASSEXW wc{};

	wc.cbSize = sizeof(wc);
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));

	wc.lpfnWndProc = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ->LRESULT {
		switch (uMsg) {
		case WM_CLOSE:
			if (MessageBoxW(hwnd, L"确定要退出程序吗？", L"提示", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
			{
				DestroyWindow(hwnd);
			}
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		};

	return wc;
}