#pragma once
#include <iostream>
#include <windows.h>
using namespace std;

namespace notification {
	class PipeClient {
	public:
		PipeClient();
		~PipeClient();
		BOOL sendAsciiString(LPCTSTR lpszPipename, char* msg);
		BOOL sendUnicodeString(LPCTSTR lpszPipename, wchar_t* msg);
		BOOL sendUtf8String(LPCTSTR lpszPipename, char* msg, int len);
	private:
		BOOL _sendData(LPCTSTR lpszPipename, byte* data, size_t len);
	};
}
using namespace notification;
