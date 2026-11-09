#include "CMemoryChecker.h"
#include "CGlobal.h"
#include <Windows.h>


CMemoryChecker::CMemoryChecker()
{
	m_pModuleChk = NULL;
}

CMemoryChecker::~CMemoryChecker()
{
}

bool CMemoryChecker::isAlreadyExist(DWORD_PTR dwAddr)
{
	for (int i = 0; i < m_addrs.size(); i++)
	{
		if (m_addrs[i] == dwAddr)
			return true;
	}
	return false;
}


void CMemoryChecker::checkMem()
{
	vector<DWORD_PTR> addrList;

	SYSTEM_INFO sysInfo = { 0 };
	GetSystemInfo(&sysInfo);
	UINT64 aNext;
	DWORD_PTR aStart = (DWORD_PTR)sysInfo.lpMinimumApplicationAddress;
	DWORD_PTR aEnd = (DWORD_PTR)sysInfo.lpMaximumApplicationAddress;

	MEMORY_BASIC_INFORMATION mbi;
	for (DWORD_PTR address = aStart; address < aEnd;)
	{
		if (m_pModuleChk->isAddressInModuleImage(address, aNext))
		{
			address = aNext;
			continue;
		}

		ZeroMemory(&mbi, sizeof(MEMORY_BASIC_INFORMATION));
		auto bytes = VirtualQuery((LPCVOID)address, &mbi, sizeof(mbi));
		if (!bytes) {
			address += 4096;
			continue;
		}
		if (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_GUARD) != PAGE_GUARD)
		{
			//check if memory has PAGE_EXECUTABLE.
			if ((mbi.Protect & PAGE_EXECUTE) ||
				(mbi.Protect & PAGE_EXECUTE_READ) ||
				(mbi.Protect & PAGE_EXECUTE_READWRITE) ||
				(mbi.Protect & PAGE_EXECUTE_WRITECOPY))
			{
				addrList.push_back((DWORD_PTR)mbi.BaseAddress);
				if(!isAlreadyExist((DWORD_PTR)mbi.BaseAddress))
					CGlobal::Log("excuting memory found addr=%p, size=%x", mbi.BaseAddress, mbi.RegionSize);
			}
		}
		address += mbi.RegionSize;
	}

	m_addrs.clear();
	for (int i = 0; i < addrList.size(); i++)
	{
		m_addrs.push_back(addrList[i]);
	}
	addrList.clear();
}
