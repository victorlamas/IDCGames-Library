#pragma once
#include <iostream>
#include <windows.h>
#include <future>
#include <stdio.h>
#include <strsafe.h>
#include <tchar.h>
#include <vector>
#include <memory>
using namespace std;

#define PIPE_NAME "\\\\.\\pipe\\IDCGamesPipe"
#define BUFF_SIZE 1024  // Reducido de 4096 a 1024 para lecturas más rápidas
#define CONNECTING_STATE 0
#define READING_STATE 1
#define WRITING_STATE 2
#define MAX_INSTANCES 1
#define PIPE_TIMEOUT 1000  // Reducido de 5000 a 1000ms para responder más rápido

namespace notification {

    // RAII wrapper for TCHAR buffers
    class SafeBuffer {
    private:
        TCHAR* buffer_;
        size_t size_;
    public:
        SafeBuffer(size_t size) : size_(size) {
            buffer_ = new TCHAR[size_];
            memset(buffer_, 0, size_ * sizeof(TCHAR));
        }
        ~SafeBuffer() {
            delete[] buffer_;
            buffer_ = nullptr;
        }
        TCHAR* get() { return buffer_; }
        const TCHAR* get() const { return buffer_; }
        size_t size() const { return size_; }

        // Disable copy to prevent double-delete
        SafeBuffer(const SafeBuffer&) = delete;
        SafeBuffer& operator=(const SafeBuffer&) = delete;
    };

    // RAII wrapper for Critical Sections
    class AutoCriticalSection {
    private:
        CRITICAL_SECTION& cs_;
        bool acquired_;
    public:
        AutoCriticalSection(CRITICAL_SECTION& cs) : cs_(cs), acquired_(false) {
            EnterCriticalSection(&cs_);
            acquired_ = true;
        }
        ~AutoCriticalSection() {
            if (acquired_) {
                LeaveCriticalSection(&cs_);
            }
        }

        // Disable copy to prevent double-release
        AutoCriticalSection(const AutoCriticalSection&) = delete;
        AutoCriticalSection& operator=(const AutoCriticalSection&) = delete;
    };

    class PipeServer {
    public:
        typedef struct
        {
            OVERLAPPED oOverlap;
            HANDLE hPipeInst;
            std::unique_ptr<SafeBuffer> chRequest;
            DWORD cbRead;
            std::unique_ptr<SafeBuffer> chReply;
            DWORD cbToWrite;
            DWORD dwState;
            BOOL  fPendingIO;
        } PIPEINST, * LPPIPEINST;

        PipeServer();
        ~PipeServer();
        bool start(const char *pipeName);
        void stop();
        bool is_running();
        void setMessageCallback(void (*callback)(TCHAR* data, size_t len));
    private:
        HANDLE _thread;
        bool _running;
        void _disconnectAndReconnect(DWORD i);
        bool _connectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo);
        void _getAnswerToRequest(LPPIPEINST pipe);
        bool _isValidHandle(HANDLE handle);  // Método para validar handles
        PIPEINST _pipes[MAX_INSTANCES];
        HANDLE   _events[MAX_INSTANCES+1];
        void (*_onMessageCallback)(TCHAR* data, size_t len);
    protected:
        void _onNewMessage(TCHAR* data, size_t len);
        static DWORD __stdcall _threadProc(LPVOID param);
    };
}
using namespace notification;
