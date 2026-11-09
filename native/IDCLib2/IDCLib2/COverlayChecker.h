#pragma once
#include <windows.h>
class COverlayChecker
{
public:
	COverlayChecker();
	~COverlayChecker();
	void check();

	static BOOL CALLBACK enumWindowsProc(HWND   hwnd,LPARAM lParam);
};
