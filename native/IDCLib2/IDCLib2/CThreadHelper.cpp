#include "CThreadHelper.h"


CThreadHelper::CThreadHelper()
{
	m_pProcessHlp = NULL;
	m_pModuleChecker = NULL;
	HMODULE hMod = GetModuleHandle("kernel32.dll");

	m_procLoadLibraryA = (DWORD_PTR)GetProcAddress(hMod, "LoadLibraryA");
	m_procLoadLibraryW = (DWORD_PTR)GetProcAddress(hMod, "LoadLibraryW");
	DWORD_PTR m_procLoadLibraryW;
}

CThreadHelper::~CThreadHelper()
{

}

DWORD_PTR CThreadHelper::getThreadStartupAddress(DWORD dwTid)
{
	HANDLE ThreadHandle = OpenThread(THREAD_QUERY_INFORMATION| THREAD_QUERY_LIMITED_INFORMATION, FALSE, dwTid);
	//HANDLE ThreadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, dwTid);
	if (ThreadHandle == NULL)
	{
		CGlobal::Log("OpenThread failed! tid=%d", dwTid);
		return 0;
	}

	PVOID StartAddress = NULL;
	if (!NT_SUCCESS(NtQueryInformationThread(ThreadHandle, ThreadQuerySetWin32StartAddress, &StartAddress, sizeof(PVOID), NULL)))
	{
		CloseHandle(ThreadHandle);
		return 0;
	}
	CloseHandle(ThreadHandle);
	return (DWORD_PTR)StartAddress;
}


bool CThreadHelper::existThread(DWORD dwTid)
{
	int nCnt = m_threads.size();
	for (int i = 0; i < nCnt; i++)
	{
		if (m_threads[i].m_dwTid == dwTid)
		{
			m_threads[i].m_bExist = TRUE;
			return true;
		}
	}
	return false;
}



void CThreadHelper::checkThreads()
{
	TREADINFO newInfo;
	CGlobal* pGlobal = CGlobal::instance();
	PVOID processes = NULL;
	if (!NT_SUCCESS(m_pProcessHlp->enumProcesses(&processes)))
		return;


	DWORD dwCurrentTid = GetCurrentThreadId();
	DWORD dwCurrentPid = GetCurrentProcessId();
	PSYSTEM_PROCESS_INFORMATION process = m_pProcessHlp->findProcessInformation(processes, dwCurrentPid);
	if (process == NULL)
	{
		free(processes);
		return;
	}

	//init state
	for (int i = 0; i < m_threads.size(); i++)
		m_threads[i].m_bExist = FALSE;


	//m_threads
	PSYSTEM_THREAD_INFORMATION threads = process->Threads;
	ULONG numberOfThreads = process->NumberOfThreads;
	DWORD dwTid;
	DWORD_PTR startUpAddr = 0, nextAddr, moduleBase;
	bool  bAddrInModule = false;
	std::string strModule;
	std::vector<DWORD_PTR> callList;

	for (ULONG i = 0; i < numberOfThreads; i++)
	{
		PSYSTEM_THREAD_INFORMATION thread = &threads[i];
		dwTid = (DWORD)thread->ClientId.UniqueThread;
		if (dwCurrentTid == dwTid) continue;
		if (existThread(dwTid))
			continue;

		//get startup addr
		startUpAddr = getThreadStartupAddress(dwTid);
		if (startUpAddr == 0)
		{
			CGlobal::Log("get thread startup addr was failed tid=%d", dwTid);
			continue;
		}

		////walkThreadStack(dwTid, callList);
		////check if it's in module region
		//
		PMODULE_INFO pModuleInfo = m_pModuleChecker->getModuleByAddress(startUpAddr);
		if (pModuleInfo)
		{
			bAddrInModule = true;
			strModule = pModuleInfo->m_strModule;
			moduleBase = (DWORD_PTR)pModuleInfo->m_hMod;
		}
		else
		{
			bAddrInModule = false;
			strModule = "";
			moduleBase = 0;
		}

		newInfo.m_dwTid = dwTid;
		newInfo.m_startAddress = startUpAddr;
		newInfo.m_bStartAddrInModule = bAddrInModule;
		strncpy(newInfo.m_strModule, strModule.c_str(), MAX_PATH);
		newInfo.m_moduleBase = moduleBase;
		newInfo.m_bExist = TRUE;

		if (bAddrInModule)
		{
			CGlobal::Log("tid=%d, startupAddr=%p, offset=+%x, module=%s", dwTid, startUpAddr, startUpAddr - moduleBase, strModule.c_str());
			if (startUpAddr == m_procLoadLibraryA || startUpAddr == m_procLoadLibraryW)
			{
				CGlobal::Log("tid=%d, startupAddr LoadLibrary ########", dwTid);
				pGlobal->addEvent("Illegal Thread (startup:LoadLibrary)=> tid=%d", dwTid);
			}
		}
		else
		{
			CGlobal::Log("tid=%d, startupAddr=%p  is not in mobule should be check!!!", dwTid, startUpAddr);
			pGlobal->addEvent("Illegal Thread (startup:Unknown)=> tid=%d", dwTid);
		}
		m_threads.push_back(newInfo);

		////display stack lists
		//for (int i = 0; i < callList.size(); i++)
		//{
		//	pModuleInfo = m_pModuleChecker->getModuleByAddress(callList[i]);
		//	if (pModuleInfo)
		//	{
		//		bAddrInModule = true;
		//		strModule = pModuleInfo->m_strModule;
		//		moduleBase = (DWORD_PTR)pModuleInfo->m_hMod;
		//		//CGlobal::logging("  callstack:, addr=%p, offset=+%x, module=%s", dwTid, startUpAddr, startUpAddr - moduleBase, strModule.c_str());
		//	}
		//	else
		//	{
		//		bAddrInModule = false;
		//		strModule = "";
		//		moduleBase = 0;
		//		//CGlobal::logging("  callstack:, addr=%p, not in module region");
		//	}
		//}
	}

	free(processes);
	callList.clear();

	//remove exit threads
	for (int i = m_threads.size() - 1; i >= 0; i--)
	{
		if (m_threads[i].m_bExist) continue;
		m_threads.erase(m_threads.begin() + i);
	}
}



void CThreadHelper::convertStackFrame(STACKFRAME *StackFrame, PPH_THREAD_STACK_FRAME ThreadStackFrame)
{
	ULONG i;
	ThreadStackFrame->PcAddress = (PVOID)StackFrame->AddrPC.Offset;
	ThreadStackFrame->ReturnAddress = (PVOID)StackFrame->AddrReturn.Offset;
	ThreadStackFrame->FrameAddress = (PVOID)StackFrame->AddrFrame.Offset;
	ThreadStackFrame->StackAddress = (PVOID)StackFrame->AddrStack.Offset;
	ThreadStackFrame->BStoreAddress = (PVOID)StackFrame->AddrBStore.Offset;

	for (i = 0; i < 4; i++)
		ThreadStackFrame->Params[i] = (PVOID)StackFrame->Params[i];
	//if (StackFrame64->FuncTableEntry)
	//	ThreadStackFrame->Flags |= PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT;
}


BOOL CThreadHelper::readMemory(HANDLE hProcess, DWORD_PTR lpBaseAddress, PVOID lpBuffer, DWORD nSize, PDWORD lpNumberOfBytesRead)
{
	*lpNumberOfBytesRead = 0;
	if (IsBadReadPtr((void*)lpBaseAddress, nSize))
		return FALSE;

	memcpy(lpBuffer, (void*)lpBaseAddress, nSize);
	*lpNumberOfBytesRead = nSize;
	return TRUE;
}

NTSTATUS CThreadHelper::walkThreadStack(DWORD dwTid, std::vector<DWORD_PTR>& callList)
{
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN suspended = FALSE;
	BOOLEAN processOpened = FALSE;
	BOOLEAN isCurrentThread = FALSE;
	BOOLEAN isSystemThread = FALSE;
	THREAD_BASIC_INFORMATION basicInfo;

	callList.clear();
	HANDLE ThreadHandle = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION, FALSE, dwTid);
	//HANDLE ThreadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, dwTid);
	if (ThreadHandle == NULL)
	{
		CGlobal::Log("walkThreadStack OpenThread failed! tid=%d", dwTid);
		return 0;
	}


	if (NT_SUCCESS(NtSuspendThread(ThreadHandle, NULL)))
		suspended = TRUE;

	STACKFRAME stackFrame;
	PH_THREAD_STACK_FRAME threadStackFrame;
	CONTEXT context;
	context.ContextFlags = CONTEXT_ALL;

	//getting thread context
	if (!NT_SUCCESS(status = NtGetContextThread(ThreadHandle, &context)))
	{
		goto SKIP;
	}


	memset(&stackFrame, 0, sizeof(STACKFRAME));
#ifdef _WIN64
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrPC.Offset = context.Rip;
	stackFrame.AddrStack.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = context.Rsp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = context.Rbp;
#else
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrPC.Offset = context.Eip;
	stackFrame.AddrStack.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = context.Esp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = context.Ebp;
#endif

	HANDLE hProcess = GetCurrentProcess();
	while (TRUE)
	{
		if(!StackWalk(
#ifdef _WIN64
			IMAGE_FILE_MACHINE_AMD64,
#else
			IMAGE_FILE_MACHINE_I386,
#endif
			hProcess,
			ThreadHandle,
			&stackFrame,
			&context,
			NULL,
			NULL,
			NULL,
			NULL
		))
			break;

		// If we have an invalid instruction pointer, break.
		if (!stackFrame.AddrPC.Offset || stackFrame.AddrPC.Offset == -1)
			break;

		// Convert the stack frame and execute the callback.
		convertStackFrame(&stackFrame, &threadStackFrame);
		callList.push_back((DWORD_PTR)threadStackFrame.PcAddress);
	}

SKIP:
	if (suspended)
		NtResumeThread(ThreadHandle, NULL);

	CloseHandle(ThreadHandle);
	return status;
}
