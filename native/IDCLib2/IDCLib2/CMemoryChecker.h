#pragma once
#include "CModuleChecker.h"
#include <vector>
using namespace std;
class CMemoryChecker
{
public:
	CMemoryChecker();
	~CMemoryChecker();
	void checkMem();

	CModuleChecker* m_pModuleChk;
	vector<DWORD_PTR> m_addrs;
	bool isAlreadyExist(DWORD_PTR dwAddr);
};
