#include "CLock.h"



CLock::CLock(CRITICAL_SECTION* pCriSection)
{
	m_pCriSection = pCriSection;
	EnterCriticalSection(m_pCriSection);
}


CLock::~CLock()
{
	LeaveCriticalSection(m_pCriSection);
}
