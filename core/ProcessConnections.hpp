#pragma once

#include <Windows.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")

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

static std::wstring IPv4ToString(DWORD address)
{
	IN_ADDR addr{};
	addr.S_un.S_addr = address;

	wchar_t buffer[INET_ADDRSTRLEN]{};
	if (InetNtopW(AF_INET, &addr, buffer, INET_ADDRSTRLEN))
		return buffer;

	return L"";
}

static std::wstring IPv6ToString(const BYTE* address)
{
	IN6_ADDR addr{};
	memcpy(&addr, address, sizeof(addr));

	wchar_t buffer[INET6_ADDRSTRLEN]{};
	if (InetNtopW(AF_INET6, &addr, buffer, INET6_ADDRSTRLEN))
		return buffer;

	return L"";
}

static std::vector<ProcessConnection> GetProcessConnections(DWORD pid)
{
	std::vector<ProcessConnection> connections;

	// TCP IPv4
	{
		DWORD size = 0;

		if (GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == ERROR_INSUFFICIENT_BUFFER)
		{
			std::vector<BYTE> buffer(size);
			auto table = reinterpret_cast<PMIB_TCPTABLE_OWNER_PID>(buffer.data());

			if (GetExtendedTcpTable(table, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR)
			{
				for (DWORD i = 0; i < table->dwNumEntries; ++i)
				{
					const auto& row = table->table[i];

					if (row.dwOwningPid != pid)
						continue;

					ProcessConnection connection{};
					connection.pid = pid;
					connection.tcp = true;
					connection.localAddress = IPv4ToString(row.dwLocalAddr);
					connection.localPort = ntohs(static_cast<u_short>(row.dwLocalPort));
					connection.remoteAddress = IPv4ToString(row.dwRemoteAddr);
					connection.remotePort = ntohs(static_cast<u_short>(row.dwRemotePort));
					connection.state = row.dwState;

					connections.push_back(std::move(connection));
				}
			}
		}
	}

	// TCP IPv6
	{
		DWORD size = 0;

		if (GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0) == ERROR_INSUFFICIENT_BUFFER)
		{
			std::vector<BYTE> buffer(size);
			auto table = reinterpret_cast<PMIB_TCP6TABLE_OWNER_PID>(buffer.data());

			if (GetExtendedTcpTable(table, &size, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR)
			{
				for (DWORD i = 0; i < table->dwNumEntries; ++i)
				{
					const auto& row = table->table[i];

					if (row.dwOwningPid != pid)
						continue;

					ProcessConnection connection{};
					connection.pid = pid;
					connection.tcp = true;
					connection.localAddress = IPv6ToString(row.ucLocalAddr);
					connection.localPort = ntohs(static_cast<u_short>(row.dwLocalPort));
					connection.remoteAddress = IPv6ToString(row.ucRemoteAddr);
					connection.remotePort = ntohs(static_cast<u_short>(row.dwRemotePort));
					connection.state = row.dwState;

					connections.push_back(std::move(connection));
				}
			}
		}
	}

	// UDP IPv4
	{
		DWORD size = 0;

		if (GetExtendedUdpTable(nullptr, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == ERROR_INSUFFICIENT_BUFFER)
		{
			std::vector<BYTE> buffer(size);
			auto table = reinterpret_cast<PMIB_UDPTABLE_OWNER_PID>(buffer.data());

			if (GetExtendedUdpTable(table, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == NO_ERROR)
			{
				for (DWORD i = 0; i < table->dwNumEntries; ++i)
				{
					const auto& row = table->table[i];

					if (row.dwOwningPid != pid)
						continue;

					ProcessConnection connection{};
					connection.pid = pid;
					connection.tcp = false;
					connection.localAddress = IPv4ToString(row.dwLocalAddr);
					connection.localPort = ntohs(static_cast<u_short>(row.dwLocalPort));

					connections.push_back(std::move(connection));
				}
			}
		}
	}

	// UDP IPv6
	{
		DWORD size = 0;

		if (GetExtendedUdpTable(nullptr, &size, FALSE, AF_INET6, UDP_TABLE_OWNER_PID, 0) == ERROR_INSUFFICIENT_BUFFER)
		{
			std::vector<BYTE> buffer(size);
			auto table = reinterpret_cast<PMIB_UDP6TABLE_OWNER_PID>(buffer.data());

			if (GetExtendedUdpTable(table, &size, FALSE, AF_INET6, UDP_TABLE_OWNER_PID, 0) == NO_ERROR)
			{
				for (DWORD i = 0; i < table->dwNumEntries; ++i)
				{
					const auto& row = table->table[i];

					if (row.dwOwningPid != pid)
						continue;

					ProcessConnection connection{};
					connection.pid = pid;
					connection.tcp = false;
					connection.localAddress = IPv6ToString(row.ucLocalAddr);
					connection.localPort = ntohs(static_cast<u_short>(row.dwLocalPort));

					connections.push_back(std::move(connection));
				}
			}
		}
	}

	return connections;
}