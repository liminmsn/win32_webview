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
HWND          g_wallpaperHwnd = nullptr;   // 壁纸窗口
HWND          g_trayHost = nullptr;   // 托盘宿主（隐藏顶层窗口）
HWND          g_mountParent = nullptr;
HANDLE        g_regThread = nullptr;
volatile bool g_watcherRun = true;
DWORD         g_lastReattach = 0;

#define WM_WALLPAPER_CHANGED (WM_APP + 1)
#define WM_TRAYICON          (WM_APP + 2)

#define IDM_TRAY_SHOW  1002
#define IDM_TRAY_EXIT  1001

NOTIFYICONDATAW g_nid = {};
BOOL g_trayAdded = FALSE;


// ============================================================
//  托盘图标
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
    else
        DBG_LOG(L"[托盘] 图标已添加\n");
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

// 右键菜单
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
        // 先销毁壁纸窗口，再销毁托盘宿主
        if (g_wallpaperHwnd && IsWindow(g_wallpaperHwnd))
            DestroyWindow(g_wallpaperHwnd);
        DestroyWindow(hTrayHost);
        break;
    }
}


// ============================================================
//  桌面挂载父窗口
// ============================================================
inline BOOL IsNewDesktopArchitecture(HWND progman) {
    return (GetWindowLongPtrW(progman, GWL_EXSTYLE) & WS_EX_NOREDIRECTIONBITMAP) != 0;
}

inline HWND GetDesktopMountParent() {
    HWND progman = FindWindowW(L"Progman", nullptr);
    if (!progman) return nullptr;

    if (IsNewDesktopArchitecture(progman)) return progman;

    DWORD_PTR r = 0;
    SendMessageTimeoutW(progman, 0x052C, 0, 0x0D, SMTO_NORMAL, 1000, &r);

    if (FindWindowExW(progman, nullptr, L"SHELLDLL_DefView", nullptr)) {
        for (HWND w = FindWindowExW(nullptr, progman, L"WorkerW", nullptr); w;
            w = FindWindowExW(nullptr, w, L"WorkerW", nullptr))
            if (!FindWindowExW(w, nullptr, L"SHELLDLL_DefView", nullptr)) return w;

        for (HWND w = FindWindowExW(nullptr, nullptr, L"WorkerW", nullptr); w;
            w = FindWindowExW(nullptr, w, L"WorkerW", nullptr))
            if (!FindWindowExW(w, nullptr, L"SHELLDLL_DefView", nullptr)) return w;

        return progman;
    }

    HWND iconW = nullptr;
    for (HWND w = FindWindowExW(nullptr, nullptr, L"WorkerW", nullptr); w;
        w = FindWindowExW(nullptr, w, L"WorkerW", nullptr))
        if (FindWindowExW(w, nullptr, L"SHELLDLL_DefView", nullptr)) { iconW = w; break; }

    if (!iconW) return progman;
    HWND wallpaperW = FindWindowExW(nullptr, iconW, L"WorkerW", nullptr);
    return wallpaperW ? wallpaperW : progman;
}


// ============================================================
//  挂载 / 重挂
// ============================================================
inline void MountToParent(HWND hwnd, HWND parent, BOOL isNewArch) {
    LONG_PTR st = GetWindowLongPtrW(hwnd, GWL_STYLE);
    st &= ~WS_POPUP;
    SetWindowLongPtrW(hwnd, GWL_STYLE, st);

    SetParent(hwnd, parent);

    st = GetWindowLongPtrW(hwnd, GWL_STYLE);
    st |= WS_CHILD;
    SetWindowLongPtrW(hwnd, GWL_STYLE, st);

    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    HWND defView = FindWindowExW(parent, nullptr, L"SHELLDLL_DefView", nullptr);
    HWND insertAfter = (isNewArch && defView) ? defView : HWND_BOTTOM;

    SetWindowPos(hwnd, insertAfter, 0, 0, w, h,
        SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED);
}

inline void ReattachWallpaper() {
    HWND hwnd = g_wallpaperHwnd;
    if (!hwnd || !IsWindow(hwnd)) return;

    DWORD now = GetTickCount();
    if (now - g_lastReattach < 1500) return;
    g_lastReattach = now;

    HWND newParent = GetDesktopMountParent();
    if (!newParent || !IsWindow(newParent)) return;

    if (newParent == g_mountParent && IsWindow(g_mountParent)) {
        HWND defView = FindWindowExW(newParent, nullptr,
            L"SHELLDLL_DefView", nullptr);
        BOOL isNew = IsNewDesktopArchitecture(newParent);
        SetWindowPos(hwnd, (isNew && defView) ? defView : HWND_BOTTOM,
            0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        return;
    }

    if (g_mountParent && IsWindow(g_mountParent))
        SetParent(hwnd, nullptr);

    LONG_PTR st = GetWindowLongPtrW(hwnd, GWL_STYLE);
    st &= ~WS_CHILD;
    st |= WS_POPUP | WS_VISIBLE;
    SetWindowLongPtrW(hwnd, GWL_STYLE, st);

    MountToParent(hwnd, newParent, IsNewDesktopArchitecture(newParent));
    g_mountParent = newParent;
}


// ============================================================
//  壁纸变更监听
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
//  创建桌面背景窗口 + 托盘宿主
// ============================================================
inline HWND CreateDesktopBackgroundWindow(LPCWSTR CLASS_NAME,
    LPCWSTR APP_NAME,
    HINSTANCE hInstance) {
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

    g_wallpaperHwnd = hwnd;
    g_mountParent = parent;
    StartWallpaperWatcher(hwnd);

    // ============================================================
    //  【新增】创建独立的隐藏顶层窗口作为托盘宿主
    //  必须是顶层（父窗口为 NULL），否则 Shell_NotifyIconW 会失败
    // ============================================================
    g_trayHost = CreateWindowExW(
        0,
        CLASS_NAME,                 // 同一个窗口类，共用 WndProc
        L"TrayHost",                // 窗口标题（不显示）
        WS_POPUP,                   // 顶层、无边框
        0, 0, 0, 0,                 // 尺寸为 0，也不 ShowWindow
        nullptr,                    // 父窗口 NULL
        nullptr,
        hInstance,
        nullptr);

    if (g_trayHost) {
        AddTrayIcon(g_trayHost, hInstance, APP_NAME);   // ★ 图标在这里显示
        DBG_LOG(L"[托盘] 宿主窗口 = %p\n", g_trayHost);
    }
    else {
        DBG_LOG(L"[托盘] 创建托盘宿主窗口失败, err = %lu\n", GetLastError());
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
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));

    wc.lpfnWndProc = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        switch (uMsg) {

            // ---------------- 托盘消息 ----------------
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

            // ---------------- 壁纸窗口消息 ----------------
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
            // 只有托盘宿主销毁时才退出消息循环
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