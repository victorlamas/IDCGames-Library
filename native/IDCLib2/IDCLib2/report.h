#pragma once
#include <windows.h>

#pragma pack(push, 1)

typedef struct _rep_string
{
	WORD   size;
	char   buff[1];
	int setString(const char* str)
	{
		size = strlen(str);
		memcpy(buff, str, size);
		return (size + 2);
	}
}REP_STRING, *PREP_STRING;


typedef struct _rep_module_info
{
	UINT64	   base;
	BYTE       dir;
	REP_STRING name;
	int setData(UINT64 addr, BYTE dirr, const char* nm)
	{
		base = addr;
		dir = dirr;
		return 9 + name.setString(nm);
	}
}REP_MODULE_INFO, *PREP_MODULE_INFO;



typedef struct _rep_thread_info
{
	DWORD  dwTid;
	UINT64 startAddr;
	UINT64 moduleBase;

	int setData(DWORD tid, UINT64 sAddr, UINT64 mAddr)
	{
		dwTid = tid;
		startAddr = sAddr;
		moduleBase = mAddr;
		return 20;
	}
}REP_THREAD_INFO, *PREP_THREAD_INFO;


typedef struct _rep_event_info
{
	DWORD  id;
	REP_STRING descr;
	int setData(DWORD Id, const char* desc)
	{
		id = Id;
		return 4 + descr.setString(desc);
	}
}REP_EVENT_INFO, *PREP_EVENT_INFO;


typedef struct _event_info
{
	DWORD  id;
	char   descr[256];
}EVENT_INFO, *PEVENT_INFO;


typedef struct _REPORT_DATA
{
	DWORD dwPid;
	BYTE  onTop;
	BYTE  bOverlay;
	BYTE  data[1];
	//char  overlayProc[MAX_PATH];
	//char  gameExe[256];
	//char  gameDir[256];
	//char  sysDir[256];
	//char  winDir[256];
	//char  progDir[256];

	//DWORD		moduleCount;
	//REP_MODULE_INFO modules[300];

	//DWORD		threadCount;
	//REP_THREAD_INFO threads[400];

	//DWORD       eventCount;
	//REP_EVENT_INFO  events[100];

}REPORT_DATA, *PREPORT_DATA;

#pragma pack(pop, 1)
