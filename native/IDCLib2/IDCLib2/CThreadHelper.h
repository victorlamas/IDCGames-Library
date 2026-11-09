#pragma once
#include <vector>
#include <string>
#include "CGlobal.h"
#include "CProcessHelper.h"
#include "CModuleChecker.h"
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")


/** Contains information about a thread stack frame. */
typedef struct _PH_THREAD_STACK_FRAME
{
    PVOID PcAddress;
    PVOID ReturnAddress;
    PVOID FrameAddress;
    PVOID StackAddress;
    PVOID BStoreAddress;
    PVOID Params[4];
    ULONG Flags;
} PH_THREAD_STACK_FRAME, *PPH_THREAD_STACK_FRAME;


typedef struct _threadInfo {
	DWORD        m_dwTid;
	UINT64		 m_startAddress;
	BOOL		 m_bStartAddrInModule;
	UINT64		 m_moduleBase;
	BOOL		 m_bExist;
	char		 m_strModule[MAX_PATH];
}TREADINFO, *PTREADINFO;


class CThreadHelper
{
public:
	CThreadHelper();
	~CThreadHelper();
	void checkThreads();
	bool existThread(DWORD dwTid);
	DWORD_PTR getThreadStartupAddress(DWORD dwTid);


	std::vector<TREADINFO> m_threads;
	CProcessHelper* m_pProcessHlp;
	CModuleChecker* m_pModuleChecker;
protected:
	DWORD_PTR m_procLoadLibraryA;
	DWORD_PTR m_procLoadLibraryW;

private:
	void convertStackFrame(STACKFRAME *StackFrame, PPH_THREAD_STACK_FRAME ThreadStackFrame);
	NTSTATUS walkThreadStack(DWORD dwTid, std::vector<DWORD_PTR>& callList);
	static BOOL __stdcall readMemory(HANDLE hProcess, DWORD_PTR lpBaseAddress, PVOID lpBuffer, DWORD nSize, PDWORD lpNumberOfBytesRead);
};
