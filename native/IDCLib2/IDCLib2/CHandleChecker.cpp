#include "CHandleChecker.h"
#include "CGlobal.h"


CHandleChecker::CHandleChecker()
{
}

CHandleChecker::~CHandleChecker()
{
	m_checkedHandles.clear();
}
void CHandleChecker::check()
{
	SYSTEM_HANDLE_INFORMATION *SystemHandleInfo = NULL;
	ULONG reqLength = 0;
	ULONG SystemInfoLength = 0;
	NTSTATUS status;

	while (
		(status = NtQuerySystemInformation(SystemHandleInformation,
			SystemHandleInfo, SystemInfoLength, &reqLength)) == STATUS_INFO_LENGTH_MISMATCH)
	{
		if(SystemHandleInfo)
			VirtualFree(SystemHandleInfo, 0, MEM_RELEASE);
		//SystemInfoLength *= 2;
		SystemInfoLength = reqLength + sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO) * 100;
		SystemHandleInfo = (PSYSTEM_HANDLE_INFORMATION)VirtualAlloc(NULL, SystemInfoLength, MEM_COMMIT, PAGE_READWRITE);
		if (SystemHandleInfo == NULL)
			return;
	}

	if (!NT_SUCCESS(status))
	{
		if (SystemHandleInfo != NULL)
			VirtualFree(SystemHandleInfo, 0, MEM_RELEASE);
		SystemHandleInfo = NULL;
		return;
	}

	for (unsigned int i = 0; i < SystemHandleInfo->NumberOfHandles; i++)
	{
		SYSTEM_HANDLE_TABLE_ENTRY_INFO SystemHandle = SystemHandleInfo->Handles[i];
		//if (pid != SystemHandle.UniqueProcessId)
		//	continue;
		if (SystemHandle.ObjectTypeIndex != 7) //process handle
			continue;
		if ((SystemHandle.GrantedAccess & PROCESS_CREATE_THREAD) ||
			(SystemHandle.GrantedAccess & PROCESS_VM_READ) ||
			(SystemHandle.GrantedAccess & PROCESS_VM_WRITE))
		{
			if (!isAlreadyChecked(SystemHandle.UniqueProcessId, SystemHandle.HandleValue))
			{

			}
		}
	}

	if (SystemHandleInfo != NULL)
		VirtualFree(SystemHandleInfo, 0, MEM_RELEASE);
}

void CHandleChecker::inspectHandle(DWORD dwPid, DWORD handle, ULONG GrantAccess)
{

}

bool CHandleChecker::isAlreadyChecked(DWORD dwPid, DWORD handle)
{
	std::vector<handleInfo> m_checkedHandles;
	for (int i = 0; i < m_checkedHandles.size(); i++)
	{
		if (m_checkedHandles[i].dwPid == dwPid && m_checkedHandles[i].Handle == handle)
			return true;
	}
	return false;
}
