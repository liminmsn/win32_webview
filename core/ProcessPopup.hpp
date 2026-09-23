#pragma once

#include <Windows.h>
#include <CommCtrl.h>
#include <Shellapi.h>
#include <vector>
#include <string>
#include <tlhelp32.h>
#include <unordered_set>
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Shell32.lib")
/// <summary>
/// 进程结构体
/// </summary>
struct ProcessInfo {
	DWORD pid;
	std::wstring name;
	std::wstring path;
	HICON icon = nullptr;
};
/// <summary>
/// 获取进程pid
/// </summary>
static std::wstring GetProcessPath(DWORD pid)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!hProcess)
		return L"";
	wchar_t path[MAX_PATH]{};
	DWORD size = MAX_PATH;
	if (!QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
		CloseHandle(hProcess);
		return L"";
	}
	CloseHandle(hProcess);
	return path;
}
/// <summary>
/// 获取进程图标
/// </summary>
static HICON GetProcessIcon(const std::wstring& path)
{
	if (path.empty())
		return nullptr;
	SHFILEINFOW fileInfo{};
	if (SHGetFileInfoW(path.c_str(), 0, &fileInfo, sizeof(fileInfo), SHGFI_ICON | SHGFI_LARGEICON))
	{
		return fileInfo.hIcon;
	}

	return nullptr;
}
/// <summary>
/// 获取进程列表
/// </summary>
static bool IsSystemProcess(DWORD pid, const std::wstring& name)
{
	if (pid == 0 || pid == 4)
		return true;

	static const std::unordered_set<std::wstring> systemProcesses = {
		L"System",
		L"Registry",
		L"smss.exe",
		L"csrss.exe",
		L"wininit.exe",
		L"services.exe",
		L"lsass.exe",
		L"svchost.exe",
		L"fontdrvhost.exe",
		L"dwm.exe",
		L"Memory Compression",
		L"Secure System",
		L"Registry",
		L"Idle"
	};

	return systemProcesses.find(name) != systemProcesses.end();
}

static std::vector<ProcessInfo> GetProcessList()
{
	std::vector<ProcessInfo> processes;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return processes;

	PROCESSENTRY32W pe32{};
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if (Process32FirstW(hSnapshot, &pe32))
	{
		do
		{
			if (IsSystemProcess(pe32.th32ProcessID, pe32.szExeFile))
				continue;

			ProcessInfo info{};
			info.pid = pe32.th32ProcessID;
			info.name = pe32.szExeFile;
			info.path = GetProcessPath(info.pid);

			if (info.path.empty())
				continue;

			info.icon = GetProcessIcon(info.path);
			processes.push_back(std::move(info));

		} while (Process32NextW(hSnapshot, &pe32));
	}

	CloseHandle(hSnapshot);
	return processes;
}


#define IDC_PROCESS_LIST 1001
#define IDC_PROCESS_OK   1002
#define IDC_PROCESS_CANCEL 1003

class ProcessPopup
{
public:
	static ProcessInfo Show(HWND owner)
	{
		ProcessPopup popup;
		return popup.ShowInternal(owner);
	}

private:
	HWND hWnd = nullptr;
	HWND hList = nullptr;
	HIMAGELIST hImageList = nullptr;
	ProcessInfo selectedProcess{};

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		ProcessPopup* popup = reinterpret_cast<ProcessPopup*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

		if (message == WM_NCCREATE)
		{
			CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
			popup = static_cast<ProcessPopup*>(cs->lpCreateParams);
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(popup));
			popup->hWnd = hWnd;
		}

		if (!popup)
			return DefWindowProcW(hWnd, message, wParam, lParam);

		switch (message)
		{
		case WM_CREATE:
			popup->CreateControls();
			return 0;

		case WM_NOTIFY:
		{
			NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lParam);
			if (nmhdr->idFrom == IDC_PROCESS_LIST && nmhdr->code == NM_DBLCLK)
				popup->SelectProcess();
			return 0;
		}

		case WM_CLOSE:
			DestroyWindow(hWnd);
			return 0;

		case WM_DESTROY:
			if (popup->hImageList)
			{
				ImageList_Destroy(popup->hImageList);
				popup->hImageList = nullptr;
			}
			return 0;
		}

		return DefWindowProcW(hWnd, message, wParam, lParam);
	}

	void CreateControls()
	{
		hList = CreateWindowExW(
			WS_EX_CLIENTEDGE,
			WC_LISTVIEWW,
			nullptr,
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
			10, 10, 460, 300,
			hWnd,
			(HMENU)IDC_PROCESS_LIST,
			GetModuleHandleW(nullptr),
			nullptr
		);

		HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		SendMessageW(hList, WM_SETFONT, (WPARAM)font, TRUE);

		ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);

		hImageList = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 64, 64);
		ListView_SetImageList(hList, hImageList, LVSIL_SMALL);

		LVCOLUMNW column{};
		column.mask = LVCF_TEXT | LVCF_WIDTH;
		column.cx = 320;
		column.pszText = const_cast<LPWSTR>(L"进程");
		SendMessageW(hList, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);

		column.cx = 100;
		column.pszText = const_cast<LPWSTR>(L"PID");
		SendMessageW(hList, LVM_INSERTCOLUMNW, 1, (LPARAM)&column);

		std::vector<ProcessInfo> processes = GetProcessList();

		for (const auto& process : processes)
		{
			int imageIndex = -1;

			if (process.icon)
				imageIndex = ImageList_AddIcon(hImageList, process.icon);

			LVITEMW item{};
			item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
			item.iItem = ListView_GetItemCount(hList);
			item.iImage = imageIndex;
			item.lParam = static_cast<LPARAM>(process.pid);
			item.pszText = const_cast<LPWSTR>(process.name.c_str());

			int index = (int)SendMessageW(hList, LVM_INSERTITEMW, 0, (LPARAM)&item);

			if (index >= 0)
			{
				wchar_t pidText[32]{};
				swprintf_s(pidText, L"%lu", process.pid);

				LVITEMW subItem{};
				subItem.iSubItem = 1;
				subItem.pszText = pidText;

				SendMessageW(hList, LVM_SETITEMTEXTW, index, (LPARAM)&subItem);
			}

			if (process.icon)
				DestroyIcon(process.icon);
		}
	}

	void SelectProcess()
	{
		int index = (int)SendMessageW(hList, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
		if (index < 0)
			return;

		LVITEMW item{};
		item.mask = LVIF_PARAM;
		SendMessageW(hList, LVM_GETITEMW, index, (LPARAM)&item);

		DWORD pid = static_cast<DWORD>(item.lParam);

		for (const auto& process : GetProcessList())
		{
			if (process.pid == pid)
			{
				selectedProcess = process;
				selectedProcess.icon = nullptr;
				break;
			}
		}

		DestroyWindow(hWnd);
	}

	ProcessInfo ShowInternal(HWND owner)
	{
		HINSTANCE hInstance = GetModuleHandleW(nullptr);

		static bool registered = false;

		if (!registered)
		{
			WNDCLASSW wc{};
			wc.lpfnWndProc = WndProc;
			wc.hInstance = hInstance;
			wc.lpszClassName = L"ProcessPopupWindow";
			wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));
			wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
			RegisterClassW(&wc);
			registered = true;
		}

		INITCOMMONCONTROLSEX icc{};
		icc.dwSize = sizeof(icc);
		icc.dwICC = ICC_LISTVIEW_CLASSES;
		InitCommonControlsEx(&icc);

		hWnd = CreateWindowExW(
			WS_EX_DLGMODALFRAME,
			L"ProcessPopupWindow",
			L"选择进程",
			WS_POPUP | WS_CAPTION | WS_SYSMENU,
			CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
			owner, nullptr, hInstance, this
		);

		if (!hWnd)
			return {};

		RECT ownerRect{}, popupRect{};
		GetWindowRect(owner, &ownerRect);
		GetWindowRect(hWnd, &popupRect);

		int width = popupRect.right - popupRect.left;
		int height = popupRect.bottom - popupRect.top;
		int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
		int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;

		SetWindowPos(hWnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
		SetForegroundWindow(hWnd);

		MSG msg{};
		while (IsWindow(hWnd) && GetMessageW(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		return selectedProcess;
	}
};