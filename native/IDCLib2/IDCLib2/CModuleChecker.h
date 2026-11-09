#pragma once

#include <map>
#include <vector>
#include <string>
#include <Windows.h>

using namespace std;


typedef struct _moduleInfo {
	UINT64  m_hMod;
	DWORD   m_dwImgSize;
	BOOL    m_bExist;
	char    m_strModule[MAX_PATH];
}MODULE_INFO, *PMODULE_INFO;


class CModuleChecker
{
public:
	CModuleChecker();
	~CModuleChecker();
	void checkModules(bool bInit=false);
	DWORD imageSize(HMODULE hMod, BOOL& bX86);
	vector<MODULE_INFO> m_modules;

	BOOL  isExistModule(HMODULE hMod);
	BOOL  isAddressInModuleImage(UINT64 addr, UINT64& nextAddr);
	PMODULE_INFO  getModuleByAddress(UINT64 addr);
};
