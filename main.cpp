#include "src/heard/AppLication.h"
#include "resources/resource.h"
#include "core/Window.hpp"
#include "global.h"

const wchar_t APP_NAME[] = L"Pktmon抓包";
const wchar_t CLASS_NAME[] = L"WinPktmonWindow";
const int width = 700;
const int height = 600;
std::unique_ptr<AppLication> application;
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
	WNDCLASSEXW wc = CreateMainWNDCLASSEXW(CLASS_NAME, hInstance);
	wc.hIcon = static_cast<HICON>(LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 256, 256, LR_DEFAULTCOLOR));
	wc.hIconSm = static_cast<HICON>(LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));
	if (!RegisterClassExW(&wc))
	{
		MessageBoxW(nullptr, L"窗口类注册失败。", L"win_pktmon", MB_ICONERROR);
		return 1;
	}

	HWND hwnd = CreateMainWindow(CLASS_NAME, APP_NAME, hInstance, width, height);
	if (!hwnd)
	{
		MessageBoxW(nullptr, L"窗口创建失败。", L"win_pktmon", MB_ICONERROR);
		return 1;
	}

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	application = std::make_unique<AppLication>(hwnd);
	application->InitWebView();

	MSG msg{};
	while (true)
	{
		BOOL result = GetMessageW(&msg, nullptr, 0, 0);
		if (result == -1) return 1;
		if (result == 0) break;

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return static_cast<int>(msg.wParam);
}