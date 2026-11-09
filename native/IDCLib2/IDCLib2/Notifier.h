#pragma once
#include "PipeServer.h"
#include "PipeClient.h"
#include "ECommands.h"
#include <windows.h>

namespace notification {
    typedef void(__stdcall* NotifyCallBack)(Command command, const wchar_t* data, int len);

    #define NOTIFY_VERSION "1.0"
    #define MAX_QUEUE_SIZE 100

    // Simple message structure to replace std::queue
    struct SimpleMessage {
        Command command;
        wchar_t* data;
        int dataLen;
        SimpleMessage* next;

        SimpleMessage(Command cmd, const wchar_t* msg, int len);
        ~SimpleMessage();
    };

    // Simple thread-safe queue using Windows CRITICAL_SECTION
    class SimpleQueue {
    private:
        SimpleMessage* head;
        SimpleMessage* tail;
        int count;
        CRITICAL_SECTION cs;

    public:
        SimpleQueue();
        ~SimpleQueue();

        bool Push(Command command, const wchar_t* data, int len);
        SimpleMessage* Pop();
        int Size();
        void Clear();
    };

    class Notifier
    {
    public:
        Notifier();
        ~Notifier();

        int Start(const wchar_t* gameId, const wchar_t* token, unsigned long ProcessId, NotifyCallBack notifyCallback, bool useQueue);
        void Stop();
        int PullMessage();
        bool SendSecure(Command command, const char* msg);
        bool IsRunning();

        // Unity crash prevention
        void SetUnitySafeMode(bool enabled) { _unitySafeMode = enabled; }
        bool IsUnitySafeMode() const { return _unitySafeMode; }

        // Método de debugging para verificar estado interno
        void LogInternalState();

        // Diagnóstico completo de comunicación
        void DiagnoseCommunication();

    private:
        void Encrypt(byte* data, int len);
        void Decrypt(byte* data, int len);
        bool SendGeneral(Command command, const char* msg);
        void SendError(int code, const wchar_t* message);
        void CleanupPreviousState();  // Nuevo método para limpiar estado anterior
        bool IsValidThreadHandle(HANDLE handle);  // Verificar si un handle es válido
    protected:
        bool _useQueue;
        PipeServer _receiver;
        PipeClient _sender;
        HANDLE _thread;

        static DWORD __stdcall _threadProc(LPVOID param);
        void   _process_message(byte* data, size_t len);

        // Unity-safe callback wrapper - no C++ objects allowed
        void   _safeCallback(Command command, wchar_t* message, int messageLen);

        void Notifier::_onPipeMessage(TCHAR* data, size_t len);

        char  _gameToken[256];
        char  _gameId[64];

        // Unity crash prevention
        bool  _unitySafeMode;

        // Stack overflow prevention
        volatile bool _processingCallback;

        // Safe thread termination
        HANDLE _terminateEvent;

        // Safe shutdown flag - atomic
        volatile bool _isShuttingDown;

        // Replace std::mutex with CRITICAL_SECTION (safer for DLL destruction)
        CRITICAL_SECTION _queueCS;
        CRITICAL_SECTION _callbackCS;
        SimpleQueue _messageQueue;
    public:
        NotifyCallBack _callback;

        static Notifier* _activeInstance;
        static void PipeMessageCallback(TCHAR* data, size_t len);
    };

    class CommandMessage {
    public:
        Command command;
        wchar_t* data;
        int dataLen;

    public:
        CommandMessage(Command _command, const wchar_t* _data, int len);
        ~CommandMessage();
    };
}
using namespace notification;
