#pragma once
#include <string>
#include <Windows.h>
class CUtil
{
public:
	CUtil();
	~CUtil();

	static bool getTagValue(LPCTSTR strContent, LPCTSTR strTag, std::string& outVal, int indexForSearch =0);
	static bool getJsonValue(LPCTSTR strContent, LPCTSTR strTag, std::string& outVal, int indexForSearch = 0);
	static void encodeUrl(LPCTSTR pStrURL, std::string& strRet);
	static void trim_str(std::string& strRet);
};
