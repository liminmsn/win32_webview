#pragma once

#include <Windows.h>
#include <dwmapi.h>
#include <shellscalingapi.h>
#include <shellapi.h>
#include <stdio.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "shell32.lib")

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
//  全局状态
// ============================================================
HWND          g_wallpaperHwnd = nullptr;
HWND          g_trayHost = nullptr;
HWND          g_mountParent = nullptr;
HANDLE        g_regThread = nullptr;
volatile bool g_watcherRun = true;
DWORD         g_lastReattach = 0;

#define WM_WALLPAPER_CHANGED (WM_APP + 1)
#define WM_TRAYICON          (WM_APP + 2)

#define IDM_TRAY_SHOW   1002
#define IDM_TRAY_EXIT   1001

NOTIFYICONDATAW g_nid = {};
BOOL g_trayAdded = FALSE;

// ============================================================
//  托盘图标管理
// ============================================================
inline BOOL AddTrayIcon(HWND hwnd, HINSTANCE hInstance, LPCWSTR tip) {
	ZeroMemory(&g_nid, sizeof(g_nid));
	g_nid.cbSize = sizeof(g_nid);
	g_nid.hWnd = hwnd;
	g_nid.uID = 1;
	g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	g_nid.uCallbackMessage = WM_TRAYICON;

	int cx = GetSystemMetrics(SM_CXSMICON);
	int cy = GetSystemMetrics(SM_CYSMICON);

	g_nid.hIcon = (HICON)LoadImageW(hInstance,
		MAKEINTRESOURCEW(IDI_APP_ICON),
		IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
	if (!g_nid.hIcon)
		g_nid.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));

	wcsncpy_s(g_nid.szTip, tip, _TRUNCATE);

	g_trayAdded = Shell_NotifyIconW(NIM_ADD, &g_nid);
	if (!g_trayAdded)
		DBG_LOG(L"[托盘] Shell_NotifyIconW 失败, err = %lu\n", GetLastError());
	return g_trayAdded;
}

inline void RemoveTrayIcon() {
	if (g_trayAdded) {
		Shell_NotifyIconW(NIM_DELETE, &g_nid);
		g_trayAdded = FALSE;
	}
	if (g_nid.hIcon) {
		DestroyIcon(g_nid.hIcon);
		g_nid.hIcon = nullptr;
	}
}

inline void ShowTrayMenu(HWND hTrayHost) {
	POINT pt;
	GetCursorPos(&pt);

	HMENU hMenu = CreatePopupMenu();
	AppendMenuW(hMenu, MF_STRING, IDM_TRAY_SHOW, L"显示/隐藏");
	AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hMenu, MF_STRING, IDM_TRAY_EXIT, L"退出");

	SetForegroundWindow(hTrayHost);

	UINT cmd = TrackPopupMenu(hMenu,
		TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
		pt.x, pt.y, 0, hTrayHost, nullptr);

	PostMessageW(hTrayHost, WM_NULL, 0, 0);
	DestroyMenu(hMenu);

	switch (cmd) {
	case IDM_TRAY_SHOW:
		if (g_wallpaperHwnd && IsWindow(g_wallpaperHwnd))
			ShowWindow(g_wallpaperHwnd,
				IsWindowVisible(g_wallpaperHwnd) ? SW_HIDE : SW_SHOW);
		break;

	case IDM_TRAY_EXIT:
		if (g_wallpaperHwnd && IsWindow(g_wallpaperHwnd))
			DestroyWindow(g_wallpaperHwnd);
		if (g_trayHost && IsWindow(g_trayHost))
			DestroyWindow(g_trayHost);
		break;
	}
}

// ============================================================
//  桌面挂载父窗口获取逻辑
//  ★ 修复：只发一次 0x052C（多次会打乱 explorer 结构）
//  ★ 修复：找不到 WorkerW 时回退到 Progman，保证窗口能创建
// ============================================================
inline HWND GetDesktopMountParent() {
	HWND progman = FindWindowW(L"Progman", nullptr);
	if (!progman) {
		DBG_LOG(L"[挂载] 找不到 Progman，explorer 没起来？\n");
		return nullptr;
	}

	// 触发 explorer 拆分出新的 WorkerW —— 只发一次标准消息
	DWORD_PTR result = 0;
	SendMessageTimeoutW(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, &result);

	// 给 explorer 一点时间把 WorkerW 建出来
	Sleep(150);

	HWND workerW = nullptr;

	EnumWindows([](HWND topHwnd, LPARAM lParam) -> BOOL {
		HWND shellView = FindWindowExW(topHwnd, nullptr, L"SHELLDLL_DefView", nullptr);
		if (!shellView) return TRUE;

		// 找到包含 SHELLDLL_DefView 的窗口后，取它后面的第一个 WorkerW
		HWND next = FindWindowExW(nullptr, topHwnd, L"WorkerW", nullptr);
		if (!next) {
			HWND cur = GetWindow(topHwnd, GW_HWNDNEXT);
			while (cur) {
				wchar_t cls[256] = { 0 };
				GetClassNameW(cur, cls, 256);
				if (wcscmp(cls, L"WorkerW") == 0) {
					next = cur;
					break;
				}
				cur = GetWindow(cur, GW_HWNDNEXT);
			}
		}
		*reinterpret_cast<HWND*>(lParam) = next;
		return FALSE;
		}, reinterpret_cast<LPARAM>(&workerW));

	if (workerW) {
		DBG_LOG(L"[挂载] 找到 WorkerW: %p\n", workerW);
		return workerW;
	}

	// ★ 回退：找不到 WorkerW 时挂到 Progman。
	//    某些 Windows 11 版本上 0x052C 不再生效，只能退到 Progman。
	//    这种情形下窗口可能被图标层覆盖，但至少程序能跑起来。
	DBG_LOG(L"[挂载] 未找到 WorkerW，回退到 Progman\n");
	return progman;
}

// ============================================================
//  挂载逻辑
// ============================================================
inline void MountToParent(HWND hwnd, HWND parent) {
	if (!hwnd || !parent) return;

	LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
	style &= ~WS_POPUP;
	style |= WS_CHILD | WS_VISIBLE;
	SetWindowLongPtrW(hwnd, GWL_STYLE, style);

	SetParent(hwnd, parent);

	int w = GetSystemMetrics(SM_CXSCREEN);
	int h = GetSystemMetrics(SM_CYSCREEN);
	POINT pt = { 0, 0 };
	ScreenToClient(parent, &pt);

	HWND shellView = FindWindowExW(parent, nullptr, L"SHELLDLL_DefView", nullptr);

	if (shellView) {
		SetWindowPos(hwnd, shellView,
			pt.x, pt.y, w, h,
			SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED);
	}
	else {
		SetWindowPos(hwnd, HWND_BOTTOM,
			pt.x, pt.y, w, h,
			SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED);
	}
}

inline void ReattachWallpaper() {
	HWND hwnd = g_wallpaperHwnd;
	if (!hwnd || !IsWindow(hwnd)) return;

	DWORD now = GetTickCount();
	if (now - g_lastReattach < 1500) return;
	g_lastReattach = now;

	HWND newParent = GetDesktopMountParent();
	if (!newParent || !IsWindow(newParent)) return;

	MountToParent(hwnd, newParent);
	g_mountParent = newParent;
}

// ============================================================
//  注册表监听
// ============================================================
inline DWORD WINAPI RegWatchThread(LPVOID param) {
	HWND hwnd = static_cast<HWND>(param);

	HKEY hKey = nullptr;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop",
		0, KEY_NOTIFY, &hKey) != ERROR_SUCCESS) {
		return 1;
	}

	HANDLE hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

	while (g_watcherRun) {
		RegNotifyChangeKeyValue(hKey, FALSE, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);
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
//  创建桌面背景窗口与托盘宿主
//  ★ 修复：加窗口类注册检查 + 详细日志
// ============================================================
inline HWND CreateDesktopBackgroundWindow(LPCWSTR CLASS_NAME, LPCWSTR APP_NAME, HINSTANCE hInstance) {
	if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
		SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

	// ---- 0. 检查窗口类是否已注册（忘了 RegisterClassExW 会在这里暴露）----
	WNDCLASSEXW wcCheck{};
	if (!GetClassInfoExW(hInstance, CLASS_NAME, &wcCheck)) {
		DBG_LOG(L"[创建] 窗口类未注册: %s, 请先 RegisterClassExW\n", CLASS_NAME);
		return nullptr;
	}

	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);

	// ---- 1. 找挂载父窗口 ----
	HWND parent = GetDesktopMountParent();
	if (!parent || !IsWindow(parent)) {
		DBG_LOG(L"[创建] 挂载父窗口无效，放弃创建\n");
		return nullptr;
	}
	{
		wchar_t cls[64]{};
		GetClassNameW(parent, cls, 64);
		DBG_LOG(L"[创建] 使用父窗口 class=%s\n", cls);
	}

	// ---- 2. 创建壁纸窗口 ----
	DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;

	HWND hwnd = CreateWindowExW(
		exStyle, CLASS_NAME, APP_NAME,
		WS_POPUP | WS_VISIBLE,
		0, 0, width, height,
		nullptr, nullptr, hInstance, nullptr);

	if (!hwnd) {
		DBG_LOG(L"[创建] CreateWindowExW 失败, err=%lu\n", GetLastError());
		return nullptr;
	}
	DBG_LOG(L"[创建] 壁纸窗口 hwnd=%p\n", hwnd);

	MountToParent(hwnd, parent);

	DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_DONOTROUND;
	DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

	g_wallpaperHwnd = hwnd;
	g_mountParent = parent;
	StartWallpaperWatcher(hwnd);

	// ---- 3. 托盘宿主 ----
	g_trayHost = CreateWindowExW(
		0, CLASS_NAME, L"TrayHost", WS_POPUP,
		0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);

	if (g_trayHost) {
		AddTrayIcon(g_trayHost, hInstance, APP_NAME);
	}
	else {
		DBG_LOG(L"[创建] 托盘宿主创建失败, err=%lu\n", GetLastError());
	}

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
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));

	wc.lpfnWndProc = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
		switch (uMsg) {

		case WM_MOUSEACTIVATE:
			if (hwnd == g_wallpaperHwnd)
				return MA_NOACTIVATE;
			break;

		case WM_WINDOWPOSCHANGING: {
			if (hwnd != g_wallpaperHwnd)
				break;

			WINDOWPOS* pos = reinterpret_cast<WINDOWPOS*>(lParam);
			if (pos && !(pos->flags & SWP_NOZORDER)) {
				HWND shellView = FindWindowExW(g_mountParent, nullptr, L"SHELLDLL_DefView", nullptr);
				pos->hwndInsertAfter = shellView ? shellView : HWND_BOTTOM;
			}
			break;
		}

		case WM_NCACTIVATE:
			if (hwnd == g_wallpaperHwnd)
				return FALSE;
			break;

		case WM_TRAYICON:
			switch (LOWORD(lParam)) {
			case WM_RBUTTONUP:
			case WM_CONTEXTMENU:
				ShowTrayMenu(hwnd);
				return 0;
			case WM_LBUTTONDBLCLK:
				if (g_wallpaperHwnd && IsWindow(g_wallpaperHwnd))
					ShowWindow(g_wallpaperHwnd,
						IsWindowVisible(g_wallpaperHwnd) ? SW_HIDE : SW_SHOW);
				return 0;
			}
			return 0;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDM_TRAY_SHOW:
				if (g_wallpaperHwnd && IsWindow(g_wallpaperHwnd))
					ShowWindow(g_wallpaperHwnd,
						IsWindowVisible(g_wallpaperHwnd) ? SW_HIDE : SW_SHOW);
				return 0;
			case IDM_TRAY_EXIT:
				if (g_wallpaperHwnd && IsWindow(g_wallpaperHwnd))
					DestroyWindow(g_wallpaperHwnd);
				if (g_trayHost && IsWindow(g_trayHost))
					DestroyWindow(g_trayHost);
				return 0;
			}
			return 0;

		case WM_ERASEBKGND:
			return TRUE;

		case WM_WALLPAPER_CHANGED:
			ReattachWallpaper();
			return 0;

		case WM_SIZE:
			if (application && hwnd == g_wallpaperHwnd)
				application->Resize(LOWORD(lParam), HIWORD(lParam));
			return 0;

		case WM_DESTROY:
			if (hwnd == g_trayHost) {
				RemoveTrayIcon();
				StopWallpaperWatcher();
				PostQuitMessage(0);
			}
			return 0;
		}
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		};

	return wc;
}