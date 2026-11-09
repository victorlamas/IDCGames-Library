#pragma once
#include "CGlobal.h"

class CProcessHelper
{
public:
	CProcessHelper();
	~CProcessHelper();
	PSYSTEM_PROCESS_INFORMATION findProcessInformation(PVOID ProcessInformation, DWORD dwPID);
	NTSTATUS CProcessHelper::enumProcesses(_Out_ PVOID *Processes);

	static bool g_bInit;
	static PPEB g_PBE;
	static bool init();
private:
	NTSTATUS CProcessHelper::enumProcessesEx(_Out_ PVOID *Processes, _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass);
};
