#include "PipeClient.h"
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <iostream>


#define SEND_BUFF_SIZE 512
PipeClient::PipeClient() {

}
PipeClient::~PipeClient() {

}

BOOL PipeClient::sendAsciiString(LPCTSTR lpszPipename, char* msg)
{
    byte data[SEND_BUFF_SIZE];
    data[0] = 0xEF;
    data[1] = 0xBB;
    data[2] = 0xBF;
    int len = strlen(msg);
    memcpy(data + 3, msg, strlen(msg));
    return _sendData(lpszPipename, data, len + 3);
}
BOOL PipeClient::sendUnicodeString(LPCTSTR lpszPipename, wchar_t* msg)
{
    byte data[SEND_BUFF_SIZE];
    data[0] = 0xEF;
    data[1] = 0xBB;
    data[2] = 0xBF;
    int len = WideCharToMultiByte(CP_UTF8, 0, msg, -1, (char*)(data + 3), SEND_BUFF_SIZE - 3, NULL, NULL);
    if (data[len -1] == 0)
        len--;
    return _sendData(lpszPipename, data, len + 3);
}

BOOL PipeClient::sendUtf8String(LPCTSTR lpszPipename, char* msg, int len)
{
    if (len > SEND_BUFF_SIZE - 3) return FALSE;
    byte data[SEND_BUFF_SIZE];
    data[0] = 0xEF;
    data[1] = 0xBB;
    data[2] = 0xBF;
    memcpy(data + 3, msg, len);
    return _sendData(lpszPipename, data, len + 3);
}

BOOL PipeClient::_sendData(LPCTSTR lpszPipename, byte* data, size_t len)
{
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    int    nTryed = 0;
    while (nTryed < 5)
    {
        hPipe = CreateFile(
            lpszPipename,   // pipe name
           // GENERIC_READ |  // read and write access
            GENERIC_WRITE,
            0,              // no sharing
            NULL,           // default security attributes
            OPEN_EXISTING,  // opens existing pipe
            FILE_ATTRIBUTE_NORMAL,              // default attributes
            NULL);          // no template file

      // Break if the pipe handle is valid.
        if (hPipe != INVALID_HANDLE_VALUE)
            break;

        // Exit if an error other than ERROR_PIPE_BUSY occurs.
        if (GetLastError() != ERROR_PIPE_BUSY)
            break;

        // All pipe instances are busy, so wait for 20 seconds.
        WaitNamedPipe(lpszPipename, 2000);
    }

    if (hPipe == INVALID_HANDLE_VALUE)
        return FALSE;

    // The pipe connected; change to message-read mode.
    DWORD dwMode = PIPE_READMODE_MESSAGE;
    BOOL fSuccess = SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL);
    if (!fSuccess)
    {
        CloseHandle(hPipe);
        return FALSE;
    }

    DWORD dwBytesWritten;
    fSuccess = WriteFile(hPipe, data, len, &dwBytesWritten, NULL);
    if (fSuccess)
        SetEndOfFile(hPipe);

    CloseHandle(hPipe);
    return fSuccess;
}
