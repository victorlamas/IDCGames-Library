#pragma once
#include <Windows.h>
#include <vector>

typedef struct _handleInfo {
	DWORD dwPid;
	DWORD Handle;
	ACCESS_MASK GrantedAccess;
}handleInfo, *PhandleInfo;

class CHandleChecker
{
public:
	CHandleChecker();
	~CHandleChecker();

	void check();
private:
	std::vector<handleInfo> m_checkedHandles;
	bool isAlreadyChecked(DWORD dwPid, DWORD handle);
	void inspectHandle(DWORD dwPid, DWORD handle, ULONG GrantAccess);
};
