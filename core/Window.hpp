#pragma once

#include <Windows.h>
#include <dwmapi.h>
#include <shellscalingapi.h>
#include <stdio.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Shcore.lib")

#include "../global.h"


// ============================================================
//  调试日志
// ============================================================
#ifdef _DEBUG
inline void DBG_LOG(const wchar_t* fmt, ...) {
	wchar_t buf[512]; va_list a; va_start(a, fmt);
	_vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, a);
	va_end(a); OutputDebugStringW(buf);
}
#else
inline void DBG_LOG(const wchar_t*, ...) {}
#endif


// ============================================================
//  重挂相关全局状态
// ============================================================
HWND          g_wallpaperHwnd = nullptr;   // 我们的壁纸窗口
HWND          g_mountParent = nullptr;   // 当前挂载的父窗口
HANDLE        g_regThread = nullptr;   // 监听线程
volatile bool g_watcherRun = true;
DWORD         g_lastReattach = 0;

#define WM_WALLPAPER_CHANGED (WM_APP + 1)


// ============================================================
//  判断是否 24H2+ 桌面
// ============================================================
inline BOOL IsNewDesktopArchitecture(HWND progman) {
	return (GetWindowLongPtrW(progman, GWL_EXSTYLE) & WS_EX_NOREDIRECTIONBITMAP) != 0;
}


// ============================================================
//  获取桌面挂载父窗口
// ============================================================
inline HWND GetDesktopMountParent() {
	HWND progman = FindWindowW(L"Progman", nullptr);
	if (!progman) return nullptr;

	// 24H2+：直接返回 Progman
	if (IsNewDesktopArchitecture(progman)) return progman;

	// 旧版本：触发 DWM 拆分
	DWORD_PTR r = 0;
	SendMessageTimeoutW(progman, 0x052C, 0, 0x0D, SMTO_NORMAL, 1000, &r);

	// 情况 A：DefView 仍在 Progman 下
	if (FindWindowExW(progman, nullptr, L"SHELLDLL_DefView", nullptr)) {
		for (HWND w = FindWindowExW(nullptr, progman, L"WorkerW", nullptr); w;
			w = FindWindowExW(nullptr, w, L"WorkerW", nullptr))
			if (!FindWindowExW(w, nullptr, L"SHELLDLL_DefView", nullptr)) return w;

		for (HWND w = FindWindowExW(nullptr, nullptr, L"WorkerW", nullptr); w;
			w = FindWindowExW(nullptr, w, L"WorkerW", nullptr))
			if (!FindWindowExW(w, nullptr, L"SHELLDLL_DefView", nullptr)) return w;

		return progman;
	}

	// 情况 B：图标层在某个 WorkerW
	HWND iconW = nullptr;
	for (HWND w = FindWindowExW(nullptr, nullptr, L"WorkerW", nullptr); w;
		w = FindWindowExW(nullptr, w, L"WorkerW", nullptr))
		if (FindWindowExW(w, nullptr, L"SHELLDLL_DefView", nullptr)) { iconW = w; break; }

	if (!iconW) return progman;
	HWND wallpaperW = FindWindowExW(nullptr, iconW, L"WorkerW", nullptr);
	return wallpaperW ? wallpaperW : progman;
}


// ============================================================
//  把窗口重新挂到当前父窗口，并压到 Z 序底部
// ============================================================
inline void MountToParent(HWND hwnd, HWND parent, BOOL isNewArch) {
	// 1. 清除 WS_POPUP
	LONG_PTR st = GetWindowLongPtrW(hwnd, GWL_STYLE);
	st &= ~WS_POPUP;
	SetWindowLongPtrW(hwnd, GWL_STYLE, st);

	// 2. 挂载
	SetParent(hwnd, parent);

	// 3. 追加 WS_CHILD
	st = GetWindowLongPtrW(hwnd, GWL_STYLE);
	st |= WS_CHILD;
	SetWindowLongPtrW(hwnd, GWL_STYLE, st);

	// 4. 定位
	int w = GetSystemMetrics(SM_CXSCREEN);
	int h = GetSystemMetrics(SM_CYSCREEN);

	HWND defView = FindWindowExW(parent, nullptr, L"SHELLDLL_DefView", nullptr);
	HWND insertAfter = (isNewArch && defView) ? defView : HWND_BOTTOM;

	SetWindowPos(hwnd, insertAfter, 0, 0, w, h,
		SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED);
}


// ============================================================
//  重挂逻辑（在主线程调用）
// ============================================================
inline void ReattachWallpaper() {
	HWND hwnd = g_wallpaperHwnd;
	if (!hwnd || !IsWindow(hwnd)) return;

	DWORD now = GetTickCount();
	if (now - g_lastReattach < 1500) return;   // 防抖
	g_lastReattach = now;

	HWND newParent = GetDesktopMountParent();
	if (!newParent || !IsWindow(newParent)) return;

	// 父窗口未变且仍有效 → 仅调整 Z 序
	if (newParent == g_mountParent && IsWindow(g_mountParent)) {
		HWND defView = FindWindowExW(newParent, nullptr,
			L"SHELLDLL_DefView", nullptr);
		BOOL isNew = IsNewDesktopArchitecture(newParent);
		SetWindowPos(hwnd, (isNew && defView) ? defView : HWND_BOTTOM,
			0, 0, 0, 0,
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
		return;
	}

	// 解挂载
	if (g_mountParent && IsWindow(g_mountParent))
		SetParent(hwnd, nullptr);

	// 恢复为顶层样式
	LONG_PTR st = GetWindowLongPtrW(hwnd, GWL_STYLE);
	st &= ~WS_CHILD;
	st |= WS_POPUP | WS_VISIBLE;
	SetWindowLongPtrW(hwnd, GWL_STYLE, st);

	// 重新挂载
	MountToParent(hwnd, newParent, IsNewDesktopArchitecture(newParent));
	g_mountParent = newParent;
}


// ============================================================
//  注册表监听线程
// ============================================================
inline DWORD WINAPI RegWatchThread(LPVOID param) {
	HWND hwnd = static_cast<HWND>(param);

	HKEY hKey = nullptr;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop",
		0, KEY_NOTIFY, &hKey) != ERROR_SUCCESS) {
		DBG_LOG(L"[监听] 打开注册表失败\n");
		return 1;
	}

	HANDLE hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

	while (g_watcherRun) {
		RegNotifyChangeKeyValue(hKey, FALSE, REG_NOTIFY_CHANGE_LAST_SET,
			hEvent, TRUE);
		if (WaitForSingleObject(hEvent, 1000) == WAIT_OBJECT_0) {
			PostMessageW(hwnd, WM_WALLPAPER_CHANGED, 0, 0);
		}
	}

	CloseHandle(hEvent);
	RegCloseKey(hKey);
	return 0;
}

inline void StartWallpaperWatcher(HWND hwnd) {
	g_watcherRun = true;
	g_regThread = CreateThread(nullptr, 0, RegWatchThread, hwnd, 0, nullptr);
}

inline void StopWallpaperWatcher() {
	g_watcherRun = false;
	if (g_regThread) {
		WaitForSingleObject(g_regThread, 2000);
		CloseHandle(g_regThread);
		g_regThread = nullptr;
	}
}


// ============================================================
//  创建桌面背景窗口
// ============================================================
inline HWND CreateDesktopBackgroundWindow(LPCWSTR CLASS_NAME, LPCWSTR APP_NAME, HINSTANCE hInstance) {
	if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
		SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);

	HWND parent = GetDesktopMountParent();
	if (!parent || !IsWindow(parent)) return nullptr;

	BOOL isNewArch = IsNewDesktopArchitecture(parent);

	DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
	if (isNewArch) exStyle |= WS_EX_LAYERED;

	HWND hwnd = CreateWindowExW(
		exStyle, CLASS_NAME, APP_NAME,
		WS_POPUP | WS_VISIBLE,
		0, 0, width, height,
		nullptr, nullptr, hInstance, nullptr);

	if (!hwnd) return nullptr;

	if (isNewArch) SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

	MountToParent(hwnd, parent, isNewArch);

	DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_DONOTROUND;
	DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
		&corner, sizeof(corner));

	// 保存全局状态 + 启动监听
	g_wallpaperHwnd = hwnd;
	g_mountParent = parent;
	StartWallpaperWatcher(hwnd);

	return hwnd;
}


// ============================================================
//  注册窗口类
// ============================================================
inline WNDCLASSEXW CreateMainWNDCLASSEXW(LPCWSTR CLASS_NAME, HINSTANCE hInstance) {
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));

	wc.lpfnWndProc = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
		switch (uMsg) {
		case WM_ERASEBKGND:
			return TRUE;

		case WM_WALLPAPER_CHANGED:
			ReattachWallpaper();
			return 0;

		case WM_SIZE:
			if (application) application->Resize(LOWORD(lParam), HIWORD(lParam));
			return 0;

		case WM_DESTROY:
			StopWallpaperWatcher();
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		};

	return wc;
}