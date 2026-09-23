#pragma once
#include <vector>
#include <tlhelp32.h>

inline static struct ProcessInfo {
	DWORD pid;
	std::wstring name;
};

// 获取系统所有进程列表
inline static std::vector<ProcessInfo> GetProcessList() {
	std::vector<ProcessInfo> processes;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		std::cerr << "创建进程快照失败！错误代码: " << GetLastError() << std::endl;
		return processes;
	}

	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	// 2. 获取第一个进程信息
	if (Process32FirstW(hSnapshot, &pe32)) {
		do {
			processes.push_back({ pe32.th32ProcessID, pe32.szExeFile });
		} while (Process32NextW(hSnapshot, &pe32)); // 3. 循环遍历后续进程
	}

	CloseHandle(hSnapshot);
	return processes;
}