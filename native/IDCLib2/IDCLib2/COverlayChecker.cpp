#include "COverlayChecker.h"
#include "CGlobal.h"

COverlayChecker::COverlayChecker()
{
}


COverlayChecker::~COverlayChecker()
{
}

BOOL CALLBACK COverlayChecker::enumWindowsProc(	HWND   hwnd,LPARAM lParam)
{
	CGlobal* pGlobal = CGlobal::instance();
	if (hwnd == NULL) return FALSE;

	RECT rc = { 0 };
	if (::GetWindowRect(hwnd, &rc))
	{
		if (rc.right - rc.left < 200) return TRUE;
		if (rc.bottom - rc.top < 200) return TRUE;
	}

	LONG dwStyle = GetWindowLongA(hwnd, GWL_STYLE);
	if (!(dwStyle & WS_VISIBLE))return TRUE;

	LONG dwExStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
	if (!(dwExStyle & WS_EX_LAYERED)) return TRUE;
	if (!(dwExStyle & WS_EX_TRANSPARENT)) return TRUE;


	COLORREF crKey;
	BYTE     bAlpha = 0;
	DWORD    dwFlags = 0;
	if (!GetLayeredWindowAttributes(hwnd, &crKey, &bAlpha, &dwFlags))
		return TRUE;

	//if ((dwFlags & LWA_ALPHA) && bAlpha < 255)
	{
		//detect overlay window
		pGlobal->m_overlay.bExist = true;
		DWORD pid = 0;
		DWORD tid = GetWindowThreadProcessId(hwnd, &pid);
		if (pid > 0)
		{
			HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
			if (hProc != NULL)
			{
				tid = MAX_PATH;
				QueryFullProcessImageNameA(hProc, 0, pGlobal->m_overlay.processName, &tid);
			}
		}

		pGlobal->addEvent("detected overlay window");
		CGlobal::Log("detected overlay window");
	}
	return TRUE;

}

void COverlayChecker::check()
{
	CGlobal* pGlobal = CGlobal::instance();
	pGlobal->m_overlay.bExist = false;
	pGlobal->m_overlay.processName[0] = 0;

	EnumWindows(enumWindowsProc, NULL);
}
