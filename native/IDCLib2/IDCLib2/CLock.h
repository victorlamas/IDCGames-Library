#pragma once
#include <Windows.h>
class CLock
{
public:
	CLock(CRITICAL_SECTION* pCriSection);
	~CLock();
	CRITICAL_SECTION* m_pCriSection;
};
