#include "CUtil.h"
#include <tchar.h>



CUtil::CUtil()
{
}


CUtil::~CUtil()
{
}

bool CUtil::getTagValue(LPCTSTR strContent, LPCTSTR strTag, std::string& outVal, int indexForSearch)
{
	char strStart[256];
	char strEnd[256];
	char value[2048];

	outVal = "";
	snprintf(strStart, 256, "<%s>", strTag);
	snprintf(strEnd, 256, "</%s>", strTag);

	char* pStart = (char*)strstr(strContent + indexForSearch, strStart);
	if (pStart == NULL) return false;

	pStart += strlen(strStart);
	char* pEnd = (char*)strstr(pStart, strEnd);
	if (pEnd == NULL) return false;

	memset(value, 0, 2048);
	memcpy(value, pStart, pEnd - pStart);
	outVal = value;
	return true;
}

bool CUtil::getJsonValue(LPCTSTR strContent, LPCTSTR strTag, std::string& outVal, int indexForSearch)
{
	char strStart[256];
	char* strEnd = ",";
	char* strEnd1 = "}";
	char value[2048];

	outVal = "";
	snprintf(strStart, 256, "\"%s\":", strTag);
	char* pStart = (char*)strstr(strContent + indexForSearch, strStart);
	if (pStart == NULL) return false;

	pStart += strlen(strStart);
	char* pEnd = (char*)strstr(pStart, strEnd);
	if (pEnd == NULL)
		pEnd = (char*)strstr(pStart, strEnd1);

	if (pEnd == NULL) return false;

	if (*pStart == '\"') pStart++;
	if (*(pEnd-1) == '\"') pEnd--;

	if (pStart > pEnd) return false;

	memset(value, 0, 2048);
	memcpy(value, pStart, pEnd - pStart);
	outVal = value;
	return true;
}


void CUtil::encodeUrl(LPCTSTR pStrURL, std::string& strRet)
{
	strRet = "";
	int nLn = lstrlen(pStrURL);
	char buff[256];

	for (int i = 0; i < nLn; i++)
	{
		TCHAR z = pStrURL[i];
		if (z == '\\')
		{
			strRet += _T('/');
		}
		else if (z == '/' ||    //There's no need to escape everything indiscriminately
			z == ':' ||
			z == '.' ||
			z == '_' ||
			(z < 0x100 && _istalnum(z)))
		{
			strRet += z;
		}
		else if (z < 0x100)
		{
			sprintf_s(buff, 256, "%%%x%x", (BYTE)(z >> 4), (BYTE)(z & 0xf));
			strRet.append(buff);
		}
		else
		{
			sprintf_s(buff, 256, "&#%u;", z);
			strRet.append(buff);
		}
	}
}

void CUtil::trim_str(std::string& strRet)
{
	bool bTrue = true;
	while (strRet.length() > 0 && bTrue)
	{
		if (strRet[0] == '\n' || strRet[0] == ' ')
		{
			strRet.erase(strRet.begin() + 0);
			bTrue = true;
		}
		else
			bTrue = false;
	}

	bTrue = true;
	while (strRet.length() > 0 && bTrue)
	{
		char ch = strRet[strRet.length() - 1];
		if (ch == '\n' || ch == ' ')
		{
			strRet.erase(strRet.begin() + strRet.length() - 1);
			bTrue = true;
		}
		else
			bTrue = false;
	}
}
