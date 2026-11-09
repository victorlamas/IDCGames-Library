#include "CProcessHelper.h"
#include "CGlobal.h"
#include "nt/ntpsapi.h"


PPEB CProcessHelper::g_PBE = NULL;

bool CProcessHelper::g_bInit = CProcessHelper::init();

CProcessHelper::CProcessHelper()
{
}


CProcessHelper::~CProcessHelper()
{
}

bool CProcessHelper::init()
{
	// FPA He comentado esto para que no inicialice.
	return false;


	PROCESS_BASIC_INFORMATION pbi;
	int nSize = sizeof(PROCESS_BASIC_INFORMATION);
	DWORD dwReturned = 0, status;
	HANDLE hProcess = GetCurrentProcess();
	ULONG_PTR wow64 = 0;

	status = NtQueryInformationProcess(hProcess,
		PROCESSINFOCLASS::ProcessBasicInformation, &pbi, nSize, &dwReturned);
	if (pbi.PebBaseAddress == NULL)
	{
		status = NtQueryInformationProcess(hProcess,
			PROCESSINFOCLASS::ProcessWow64Information, &wow64, sizeof(ULONG_PTR), NULL);
		pbi.PebBaseAddress = (PPEB)wow64;
	}
	g_PBE = (PPEB)(pbi.PebBaseAddress);

	if (g_PBE)
	{
		CGlobal::Log("PEB addr=%p, pLdr=%p", g_PBE, g_PBE->Ldr);
		return true;
	}

	return false;
}


NTSTATUS CProcessHelper::enumProcessesEx(_Out_ PVOID *Processes,_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass)
{
	static ULONG initialBufferSize[3] = { 0x4000, 0x4000, 0x4000 };
	NTSTATUS status;
	ULONG classIndex;
	PVOID buffer;
	ULONG bufferSize;

	switch (SystemInformationClass)
	{
	case SystemProcessInformation:
		classIndex = 0;
		break;
	case SystemExtendedProcessInformation:
		classIndex = 1;
		break;
	case SystemFullProcessInformation:
		classIndex = 2;
		break;
	default:
		return STATUS_INVALID_INFO_CLASS;
	}

	bufferSize = initialBufferSize[classIndex];
	buffer = malloc(bufferSize);

	while (TRUE)
	{
		status = NtQuerySystemInformation(
			SystemInformationClass,
			buffer,
			bufferSize,
			&bufferSize
		);

		if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
		{
			free(buffer);
			buffer = malloc(bufferSize);
		}
		else
		{
			break;
		}
	}

	if (!NT_SUCCESS(status))
	{
		free(buffer);
		return status;
	}

	if (bufferSize <= 0x100000) initialBufferSize[classIndex] = bufferSize;
	*Processes = buffer;
	return status;
}


NTSTATUS CProcessHelper::enumProcesses(_Out_ PVOID *Processes)
{
	return enumProcessesEx(Processes, SystemProcessInformation);
}


PSYSTEM_PROCESS_INFORMATION CProcessHelper::findProcessInformation(PVOID ProcessInformation, DWORD dwPID)
{
	PSYSTEM_PROCESS_INFORMATION process;
	process = PH_FIRST_PROCESS(ProcessInformation);
	do
	{
		if ((DWORD)process->UniqueProcessId == dwPID)
			return process;
	} while (process = PH_NEXT_PROCESS(process));

	return NULL;
}
