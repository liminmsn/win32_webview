#pragma once
#include <Windows.h>

// 检查是否具备管理员权限（Pktmon 必须以管理员权限运行）
inline static bool IsRunAsAdmin() {
	BOOL isAdmin = FALSE;
	PSID adminGroup = NULL;
	SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
	if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
		CheckTokenMembership(NULL, adminGroup, &isAdmin);
		FreeSid(adminGroup);
	}
	return isAdmin;
}
