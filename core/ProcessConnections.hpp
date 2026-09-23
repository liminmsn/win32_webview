#pragma once

// 引入系统 SDK 版本解析
#include <sdkddkver.h>

// Winsock 与 IP Helper API 头文件 (必须严格保持此引入顺序)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>

#include <vector>
#include <string>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")



struct ProcessConnection
{
	DWORD pid = 0;
	bool tcp = false;

	std::wstring localAddress;
	UINT16 localPort = 0;

	std::wstring remoteAddress;
	UINT16 remotePort = 0;

	DWORD state = 0;
};

// IPv4 地址转字符串
static std::wstring IPv4ToString(DWORD address)
{
	IN_ADDR addr{};
	addr.S_un.S_addr = address;

	wchar_t buffer[INET_ADDRSTRLEN]{};
	if (InetNtopW(AF_INET, &addr, buffer, INET_ADDRSTRLEN))
		return buffer;

	return L"";
}

// IPv6 地址转字符串
static std::wstring IPv6ToString(const UCHAR address[16])
{
	IN6_ADDR addr{};
	memcpy(&addr, address, sizeof(IN6_ADDR));

	wchar_t buffer[INET6_ADDRSTRLEN]{};
	if (InetNtopW(AF_INET6, &addr, buffer, INET6_ADDRSTRLEN))
		return buffer;

	return L"";
}

static std::vector<ProcessConnection> GetProcessConnections(DWORD pid)
{
	std::vector<ProcessConnection> connections;

	// 1. TCP IPv4
	{
		DWORD size = 0;
		if (GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == ERROR_INSUFFICIENT_BUFFER)
		{
			std::vector<BYTE> buffer(size);
			auto table = reinterpret_cast<MIB_TCPTABLE_OWNER_PID*>(buffer.data());
			if (GetExtendedTcpTable(table, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR)
			{
				for (DWORD i = 0; i < table->dwNumEntries; ++i)
				{
					const auto& row = table->table[i];
					if (row.dwOwningPid != pid) continue;

					ProcessConnection conn{};
					conn.pid = pid;
					conn.tcp = true;
					conn.localAddress = IPv4ToString(row.dwLocalAddr);
					conn.localPort = ntohs(static_cast<u_short>(row.dwLocalPort));
					conn.remoteAddress = IPv4ToString(row.dwRemoteAddr);
					conn.remotePort = ntohs(static_cast<u_short>(row.dwRemotePort));
					conn.state = row.dwState;
					connections.push_back(std::move(conn));
				}
			}
		}
	}

	// 2. TCP IPv6
	{
		DWORD size = 0;
		if (GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0) == ERROR_INSUFFICIENT_BUFFER)
		{
			std::vector<BYTE> buffer(size);
			auto table = reinterpret_cast<MIB_TCP6TABLE_OWNER_PID*>(buffer.data());
			if (GetExtendedTcpTable(table, &size, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR)
			{
				for (DWORD i = 0; i < table->dwNumEntries; ++i)
				{
					const auto& row = table->table[i];
					if (row.dwOwningPid != pid) continue;

					ProcessConnection conn{};
					conn.pid = pid;
					conn.tcp = true;
					conn.localAddress = IPv6ToString(row.ucLocalAddr);
					conn.localPort = ntohs(static_cast<u_short>(row.dwLocalPort));
					conn.remoteAddress = IPv6ToString(row.ucRemoteAddr);
					conn.remotePort = ntohs(static_cast<u_short>(row.dwRemotePort));
					conn.state = row.dwState;
					connections.push_back(std::move(conn));
				}
			}
		}
	}

	// 3. UDP IPv4
	{
		DWORD size = 0;
		if (GetExtendedUdpTable(nullptr, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == ERROR_INSUFFICIENT_BUFFER)
		{
			std::vector<BYTE> buffer(size);
			auto table = reinterpret_cast<MIB_UDPTABLE_OWNER_PID*>(buffer.data());
			if (GetExtendedUdpTable(table, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == NO_ERROR)
			{
				for (DWORD i = 0; i < table->dwNumEntries; ++i)
				{
					const auto& row = table->table[i];
					if (row.dwOwningPid != pid) continue;

					ProcessConnection conn{};
					conn.pid = pid;
					conn.tcp = false;
					conn.localAddress = IPv4ToString(row.dwLocalAddr);
					conn.localPort = ntohs(static_cast<u_short>(row.dwLocalPort));
					connections.push_back(std::move(conn));
				}
			}
		}
	}

	// 4. UDP IPv6
	{
		DWORD size = 0;
		if (GetExtendedUdpTable(nullptr, &size, FALSE, AF_INET6, UDP_TABLE_OWNER_PID, 0) == ERROR_INSUFFICIENT_BUFFER)
		{
			std::vector<BYTE> buffer(size);
			auto table = reinterpret_cast<MIB_TCP6TABLE_OWNER_PID*>(buffer.data());
			if (GetExtendedUdpTable(table, &size, FALSE, AF_INET6, UDP_TABLE_OWNER_PID, 0) == NO_ERROR)
			{
				for (DWORD i = 0; i < table->dwNumEntries; ++i)
				{
					const auto& row = table->table[i];
					if (row.dwOwningPid != pid) continue;

					ProcessConnection conn{};
					conn.pid = pid;
					conn.tcp = false;
					conn.localAddress = IPv6ToString(row.ucLocalAddr);
					conn.localPort = ntohs(static_cast<u_short>(row.dwLocalPort));
					connections.push_back(std::move(conn));
				}
			}
		}
	}

	return connections;
}

static const wchar_t* TcpStateToString(DWORD state)
{
	switch (state)
	{
	case MIB_TCP_STATE_CLOSED:      return L"CLOSED";
	case MIB_TCP_STATE_LISTEN:      return L"LISTEN";
	case MIB_TCP_STATE_SYN_SENT:    return L"SYN_SENT";
	case MIB_TCP_STATE_SYN_RCVD:    return L"SYN_RCVD";
	case MIB_TCP_STATE_ESTAB:       return L"ESTAB";
	case MIB_TCP_STATE_FIN_WAIT1:   return L"FIN_WAIT1";
	case MIB_TCP_STATE_FIN_WAIT2:   return L"FIN_WAIT2";
	case MIB_TCP_STATE_CLOSE_WAIT:  return L"CLOSE_WAIT";
	case MIB_TCP_STATE_CLOSING:     return L"CLOSING";
	case MIB_TCP_STATE_LAST_ACK:    return L"LAST_ACK";
	case MIB_TCP_STATE_TIME_WAIT:   return L"TIME_WAIT";
	case MIB_TCP_STATE_DELETE_TCB:  return L"DELETE_TCB";
	default:                        return L"UNKNOWN";
	}
}

// 把连接列表拼成一段可读文本
static std::wstring FormatConnections(const std::vector<ProcessConnection>& connections)
{
	if (connections.empty())
		return L"（该进程没有网络连接）";

	std::wstring text;
	text += L"连接数: " + std::to_wstring(connections.size()) + L"\n\n";

	int index = 1;
	for (const auto& c : connections)
	{
		text += L"[" + std::to_wstring(index++) + L"] ";
		text += c.tcp ? L"TCP  " : L"UDP  ";

		// 本地地址
		text += L"本地 " + c.localAddress + L":" + std::to_wstring(c.localPort);

		// 远端地址（UDP 没有远端，端口为 0 时跳过）
		if (c.tcp || c.remotePort != 0)
		{
			text += L"  ->  远端 " + c.remoteAddress + L":" + std::to_wstring(c.remotePort);
		}

		// 状态（只有 TCP 有意义）
		if (c.tcp)
		{
			text += L"  ";
			text += TcpStateToString(c.state);
		}

		text += L"\n";
	}
	return text;
}