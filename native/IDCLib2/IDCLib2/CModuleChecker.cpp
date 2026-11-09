#include "CModuleChecker.h"
#include "CProcessHelper.h"
#include "CGlobal.h"
#pragma comment (lib, "psapi.lib")
#include <Psapi.h>


CModuleChecker::CModuleChecker()
{
}


CModuleChecker::~CModuleChecker()
{
}

void CModuleChecker::checkModules(bool bInit)
{
	MODULE_INFO newInfo;
	CGlobal* pGlobal = CGlobal::instance();
	BOOL bX86;
	char buff[512];
	if (!CProcessHelper::g_bInit)
		return;

	//init module list exist flag
	for (int i = 0; i < m_modules.size(); i++)
		m_modules[i].m_bExist = FALSE;

	PPEB peb = CProcessHelper::g_PBE;
	PLDR_DATA_TABLE_ENTRY current = (PLDR_DATA_TABLE_ENTRY)peb->Ldr->InLoadOrderModuleList.Flink;

	while (current != NULL && current != (PLDR_DATA_TABLE_ENTRY)&peb->Ldr->InLoadOrderModuleList)
	{
		if (bInit || !isExistModule((HMODULE)current->DllBase))
		{
			sprintf(buff, "%S", current->FullDllName.Buffer);
			string strPathName = buff;

			if(bInit)
				CGlobal::Log("%p: %s", current->DllBase, strPathName.c_str());
			else
			{
				CGlobal::Log("New Module Detected %p: %s", current->DllBase, strPathName.c_str());
				pGlobal->formatPathName(strPathName.c_str(), buff);
				pGlobal->addEvent("New module=> %s", buff);
			}


			newInfo.m_hMod = (UINT64)current->DllBase;
			newInfo.m_dwImgSize = current->SizeOfImage;
			strncpy(newInfo.m_strModule, strPathName.c_str(), MAX_PATH);
			m_modules.push_back(newInfo);
		}
		current = (PLDR_DATA_TABLE_ENTRY)current->InLoadOrderLinks.Flink;
	}

	//remove unload modules
	for (int i = m_modules.size()-1; i >=0 ; i--)
	{
		if (m_modules[i].m_bExist) continue;
		m_modules.erase(m_modules.begin() + i);
	}


}

#define IMAGE_NT_OPTIONAL_HDR32_MAGIC      0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC      0x20b
DWORD CModuleChecker::imageSize(HMODULE hMod, BOOL& bX86)
{
	IMAGE_DOS_HEADER* pDosHeader = (IMAGE_DOS_HEADER*)hMod;

	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return 0;

	IMAGE_FILE_HEADER* pFileHdr = (IMAGE_FILE_HEADER*)((uintptr_t)hMod + pDosHeader->e_lfanew + sizeof(DWORD));

	IMAGE_OPTIONAL_HEADER32* pOptHdr32;
	IMAGE_OPTIONAL_HEADER64* pOptHdr64;

	if (pFileHdr->SizeOfOptionalHeader == 0)
		return 0;

	pOptHdr32 = (IMAGE_OPTIONAL_HEADER32*)((uintptr_t)pFileHdr + sizeof(IMAGE_FILE_HEADER));
	pOptHdr64 = (IMAGE_OPTIONAL_HEADER64*)pOptHdr32;

	if (IMAGE_NT_OPTIONAL_HDR32_MAGIC == pOptHdr32->Magic)
	{
		bX86 = TRUE;
		return pOptHdr32->SizeOfImage;
	}
	else if (IMAGE_NT_OPTIONAL_HDR64_MAGIC == pOptHdr32->Magic)
	{
		bX86 = FALSE;
		return pOptHdr64->SizeOfImage;
	}
	return 0;
}
BOOL  CModuleChecker::isExistModule(HMODULE hMod)
{
	int nSize = m_modules.size();
	for (int i = 0; i < nSize; i++)
	{
		if (m_modules[i].m_hMod == (UINT64)hMod)
		{
			m_modules[i].m_bExist = TRUE;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL  CModuleChecker::isAddressInModuleImage(UINT64 addr, UINT64& nextAddr)
{
	UINT64 base;
	int nSize = m_modules.size();
	for (int i = 0; i < nSize; i++)
	{
		base = m_modules[i].m_hMod;
		if (addr < base) continue;
		if(addr >= base + m_modules[i].m_dwImgSize)continue;
		nextAddr = base + m_modules[i].m_dwImgSize;
		return TRUE;
	}
	return FALSE;
}

PMODULE_INFO  CModuleChecker::getModuleByAddress(UINT64 addr)
{
	UINT64 base;
	int nSize = m_modules.size();
	for (int i = 0; i < nSize; i++)
	{
		base = m_modules[i].m_hMod;
		if (addr < base) continue;
		if (addr >= base + m_modules[i].m_dwImgSize)continue;
		return &m_modules[i];
	}
	return NULL;

}
