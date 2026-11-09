#include "base64.h"
#include "md5.h"
#include "Notifier.h"
#include "CGlobal.h"

// Static instance pointer
Notifier* Notifier::_activeInstance = nullptr;

// Static callback function for PipeServer
void Notifier::PipeMessageCallback(TCHAR* data, size_t len) {
    //CGlobal::Log("PipeMessageCallback called");

    if (_activeInstance) {
        _activeInstance->_onPipeMessage(data, len);
    }
    /*else {
        //CGlobal::Log("_activeInstance is null");
    }*/
}

// SimpleMessage implementation
SimpleMessage::SimpleMessage(Command cmd, const wchar_t* msg, int len) : command(cmd), dataLen(len), next(nullptr) {
    // len is the payload size in bytes (UTF-16LE), not wchar count.
    if (msg && len > 0) {
        int wcharCount = len / (int)sizeof(wchar_t);
        data = new wchar_t[wcharCount + 1];
        memcpy(data, msg, len);
        data[wcharCount] = 0;
    } else {
        data = nullptr;
    }
}

SimpleMessage::~SimpleMessage() {
    if (data) {
        delete[] data;
        data = nullptr;
    }
}

// SimpleQueue implementation
SimpleQueue::SimpleQueue() : head(nullptr), tail(nullptr), count(0) {
    InitializeCriticalSection(&cs);
}

SimpleQueue::~SimpleQueue() {
    Clear();
    DeleteCriticalSection(&cs);
}

bool SimpleQueue::Push(Command command, const wchar_t* data, int len) {
    if (count >= MAX_QUEUE_SIZE) {
        return false; // Queue full
    }

    SimpleMessage* msg = new SimpleMessage(command, data, len);

    EnterCriticalSection(&cs);
    if (tail) {
        tail->next = msg;
        tail = msg;
    } else {
        head = tail = msg;
    }
    count++;
    LeaveCriticalSection(&cs);

    return true;
}

SimpleMessage* SimpleQueue::Pop() {
    EnterCriticalSection(&cs);
    SimpleMessage* result = nullptr;

    if (head) {
        result = head;
        head = head->next;
        if (!head) {
            tail = nullptr;
        }
        count--;
        result->next = nullptr; // Disconnect from queue
    }

    LeaveCriticalSection(&cs);
    return result;
}

int SimpleQueue::Size() {
    EnterCriticalSection(&cs);
    int size = count;
    LeaveCriticalSection(&cs);
    return size;
}

void SimpleQueue::Clear() {
    EnterCriticalSection(&cs);
    while (head) {
        SimpleMessage* temp = head;
        head = head->next;
        delete temp;
    }
    head = tail = nullptr;
    count = 0;
    LeaveCriticalSection(&cs);
}

Notifier::Notifier(){
    _callback = NULL;
    _useQueue = false;
    _unitySafeMode = true;  // Enable Unity protection by default
    _processingCallback = false;  // Initialize recursion guard
    _isShuttingDown = false;  // Initialize shutdown flag

    // Initialize critical sections
    InitializeCriticalSection(&_queueCS);
    InitializeCriticalSection(&_callbackCS);

    // Create termination event for safe thread shutdown
    _terminateEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // Inicializar buffers
    memset(_gameToken, 0, sizeof(_gameToken));
    memset(_gameId, 0, sizeof(_gameId));

    //CGlobal::Log("Notifier constructor called with Unity safe mode enabled");
}
Notifier::~Notifier(){
    //CGlobal::Log("Notifier destructor called");

    // CRITICAL: Set shutdown flag first to prevent new operations
    _isShuttingDown = true;

    // Usar el método de limpieza completo en el destructor
    try {
        CleanupPreviousState();

        // Clean up termination event
        if (_terminateEvent != NULL) {
            CloseHandle(_terminateEvent);
            _terminateEvent = NULL;
        }

        // Clean up critical sections last
        DeleteCriticalSection(&_callbackCS);
        DeleteCriticalSection(&_queueCS);

        //CGlobal::Log("Notifier destructor completed successfully");
    }
    catch (...) {
        //CGlobal::Log("Exception in Notifier destructor");
    }
}

void Notifier::Decrypt(byte* inBuff, int len){
    if (inBuff == NULL || len <= 0) {
        //CGlobal::Log("Decrypt: Invalid parameters");
        return;
    }

    // CRITICAL: Add upper bounds validation
    if (len > 65536) {  // Reasonable maximum size
        //CGlobal::Log("Decrypt: Buffer too large: %d bytes", len);
        return;
    }

    const char* cheaper = _gameToken;
    int nCheaperLen = strlen(cheaper);

    if (nCheaperLen == 0) {
        //CGlobal::Log("Decrypt: No token available for decryption");
        return;
    }

    int prevData = 0;
    for (int i = 0; i < len; i++) {
        int seed = (int)cheaper[i % nCheaperLen];
        int data = (unsigned char)inBuff[i];
        data -= seed;
        data -= prevData;
        while (data < 0) data += 256;
        prevData = data;
        inBuff[i] = data;
    }
    inBuff[len] = 0;
}

void Notifier::Encrypt(byte* inBuff, int len) {
    if (inBuff == NULL || len <= 0) {
        //CGlobal::Log("Encrypt: Invalid parameters");
        return;
    }

    // CRITICAL: Add upper bounds validation
    if (len > 65536) {  // Reasonable maximum size
        //CGlobal::Log("Encrypt: Buffer too large: %d bytes", len);
        return;
    }

    const char* cheaper = _gameToken;
    int nCheaperLen = strlen(cheaper);

    if (nCheaperLen == 0) {
        //CGlobal::Log("Encrypt: No token available for encryption");
        return;
    }

    int prevData = 0;

    for (int i = 0; i < len; i++) {
        int seed = (int)cheaper[i % nCheaperLen];
        int data = inBuff[i];
        data += seed;
        data += prevData;
        while (data > 255) data -= 256;
        prevData = inBuff[i];
        inBuff[i] = data;
    }
}

bool Notifier::IsValidThreadHandle(HANDLE handle) {
    if (handle == NULL || handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD exitCode;
    if (!GetExitCodeThread(handle, &exitCode)) {
        //CGlobal::Log("GetExitCodeThread failed - handle invalid");
        return false;
    }

    return true;
}

void Notifier::CleanupPreviousState() {
    try {
        //CGlobal::Log("Cleaning up previous state...");

        // CRITICAL: Set shutdown flag immediately to stop all operations
        _isShuttingDown = true;

        // Detener el receiver si está ejecutándose
        if (_receiver.is_running()) {
            //CGlobal::Log("Stopping previous receiver...");
            _receiver.stop();
        }


        // Limpiar callback safely
        try {
            EnterCriticalSection(&_callbackCS);
            _callback = NULL;
            LeaveCriticalSection(&_callbackCS);
        }
        catch (...) {
            //CGlobal::Log("Exception accessing callback CS during cleanup");
            _callback = NULL; // Set directly if CS fails
        }

        // Limpiar cola de mensajes safely
        try {
            _messageQueue.Clear();
        }
        catch (...) {
            //CGlobal::Log("Exception clearing message queue during cleanup");
        }

        // Reinicializar tokens y buffers
        memset(_gameToken, 0, sizeof(_gameToken));
        memset(_gameId, 0, sizeof(_gameId));

        //CGlobal::Log("Previous state cleaned successfully");
    }
    catch (...) {
        //CGlobal::Log("Cleanup error occurred, continuing...");
    }
}

int Notifier::Start(const wchar_t* gameId, const wchar_t* secret, unsigned long ProcessId, NotifyCallBack notifyCallback, bool useQueue) {
    // CRITICAL: Input validation to prevent buffer overflows
    if (gameId == nullptr || secret == nullptr) {
        //CGlobal::Log("Start: Invalid input parameters (null pointers)");
        return -100;
    }

    size_t gameIdLen = wcslen(gameId);
    size_t secretLen = wcslen(secret);

    if (gameIdLen == 0 || gameIdLen >= sizeof(_gameId)) {
        //CGlobal::Log("Start: Invalid gameId length: %zu (max: %zu)", gameIdLen, sizeof(_gameId) - 1);
        return -101;
    }

    if (secretLen == 0 || secretLen >= 64) {  // _gameSecret buffer size
        //CGlobal::Log("Start: Invalid secret length: %zu (max: 63)", secretLen);
        return -102;
    }

    if (notifyCallback == nullptr) {
        //CGlobal::Log("Start: Invalid callback function");
        return -103;
    }

    // ✅ AGREGAR: Limpiar estado anterior antes de empezar
    CleanupPreviousState();

    // CRITICAL: Reset shutdown flag for new session
    _isShuttingDown = false;

    // Reset termination event
    if (_terminateEvent != NULL) {
        ResetEvent(_terminateEvent);
    }

    //CGlobal::Log("[NOTIFIER] === Starting Notifier Initialization ===");
    //CGlobal::Log("[NOTIFIER] ProcessId: %d, UseQueue: %s", ProcessId, useQueue ? "true" : "false");
    //CGlobal::Log("[NOTIFIER] Callback pointer: %p", notifyCallback);
    //CGlobal::Log("[NOTIFIER] Unity safe mode: %s", _unitySafeMode ? "enabled" : "disabled");

    // Log queue state
    EnterCriticalSection(&_queueCS);
    //CGlobal::Log("[NOTIFIER] Initial queue size: %d", _messageQueue.Size());
    LeaveCriticalSection(&_queueCS);

    // Set callback with proper locking
    EnterCriticalSection(&_callbackCS);
    _callback = notifyCallback;
    LeaveCriticalSection(&_callbackCS);
    //CGlobal::Log("[NOTIFIER] Callback registered successfully");

    _useQueue = useQueue;

    WideCharToMultiByte(CP_UTF8, 0, gameId, -1, _gameId, 64, NULL, NULL);
    char _gameSecret[64];
    WideCharToMultiByte(CP_UTF8, 0, secret, -1, _gameSecret, 64, NULL, NULL);

    char fullPipename[64];
    sprintf_s(fullPipename, 64, "WFByW8%sLEbhl9Gm4F", _gameId);
    strcpy(_gameToken, MD5String(fullPipename));

    // Open pipe.
    sprintf_s(fullPipename, 64, "%s_%s", PIPE_NAME, _gameId);
    //CGlobal::Log("[NOTIFIER] Starting receiver with pipe: %s", fullPipename);
    //CGlobal::Log("[NOTIFIER] Current thread ID: %d", GetCurrentThreadId());

    DWORD initStartTime = GetTickCount64();
    DWORD currentTime;

    if (_receiver.start(fullPipename)) {
        // Register callback for raw message delivery
        _activeInstance = this;
        _receiver.setMessageCallback(&Notifier::PipeMessageCallback);

        currentTime = GetTickCount64();
        //CGlobal::Log("[NOTIFIER] Receiver started successfully in %d ms", currentTime - initStartTime);

        //CGlobal::Log("[NOTIFIER] Creating communication thread...");

        // Reset termination event for new session
        if (_terminateEvent != NULL) {
            ResetEvent(_terminateEvent);
            //CGlobal::Log("[NOTIFIER] Termination event reset successfully");
        } /* else {
            //CGlobal::Log("[NOTIFIER] WARNING: Termination event is NULL");
        }*/

        currentTime = GetTickCount64();
        //CGlobal::Log("[NOTIFIER] Total initialization time: %d ms", currentTime - initStartTime);
    }
    else {
        SendError(-2, L"Can't open Comm.");
        //CGlobal::Log("Notifier::Can't open Comm.");
        return -2;
    }
    //CGlobal::Log("[NOTIFIER] Sending initialization data...");
    char data[128];
    sprintf_s(data, 128, "{\"gameSecret\":\"%s\", \"processID\":%d }", _gameSecret, ProcessId);

    DWORD sendStartTime = GetTickCount64();
    bool ret = SendSecure(Command::InitLib, data);
    currentTime = GetTickCount64();
    //CGlobal::Log("[NOTIFIER] Send operation took: %d ms", currentTime - sendStartTime);

    if (!ret) {
        //CGlobal::Log("[NOTIFIER] ERROR: Initial connection failed (Time: %d ms)", currentTime - sendStartTime);
        //CGlobal::Log("[NOTIFIER] Starting cleanup procedure...");

        if (_receiver.is_running()) {
            //CGlobal::Log("[NOTIFIER] Stopping receiver...");
            _receiver.stop();
        }


        SendError(-1, L"Can't connect");
        //CGlobal::Log("Notifier::Can't connect.");
        return -1;
    }
    return 0;
}

void Notifier::Stop() {
    //CGlobal::Log("=== STOP CALLED ===");

    // CRITICAL: Set shutdown flag immediately
    _isShuttingDown = true;

    char data[8] = "{}";
	SendSecure(Command::EndLib, data); // Send EndLib command to launcher.

	// Stop the receiver if it is running.
    if (_receiver.is_running()) {
        //CGlobal::Log("Stopping receiver...");
        _receiver.stop();
    }


    // Wait a bit more to ensure thread has completely finished
    Sleep(100);

	// Remove callback after thread is stopped - with exception handling
    try {
        EnterCriticalSection(&_callbackCS);
        _callback = NULL;
        LeaveCriticalSection(&_callbackCS);
    }
    catch (...) {
        //CGlobal::Log("Exception accessing callback CS in Stop()");
        _callback = NULL; // Set directly if CS fails
    }

    // Clear the queue to prevent memory leaks - with exception handling
    try {
        _messageQueue.Clear();
    }
    catch (...) {
        //CGlobal::Log("Exception clearing message queue in Stop()");
    }

    //CGlobal::Log("=== STOP COMPLETED ===");
}

bool Notifier::IsRunning() {
    return _receiver.is_running();
}

void Notifier::LogInternalState() {
    //CGlobal::Log("=== Notifier Internal State ===");

    //CGlobal::Log("Receiver running: %s", _receiver.is_running() ? "YES" : "NO");
    //CGlobal::Log("Callback set: %s", (_callback != NULL) ? "YES" : "NO");
    //CGlobal::Log("Use queue: %s", _useQueue ? "YES" : "NO");
    //CGlobal::Log("Game ID: %s", _gameId);

    //CGlobal::Log("Notifier queue size: %d", _messageQueue.Size());

    //CGlobal::Log("Game token length: %d", strlen(_gameToken));
    //CGlobal::Log("Shutdown flag: %s", _isShuttingDown ? "YES" : "NO");
    //CGlobal::Log("Processing callback: %s", _processingCallback ? "YES" : "NO");
    //CGlobal::Log("Unity safe mode: %s", _unitySafeMode ? "YES" : "NO");
    //CGlobal::Log("===============================");
}

void Notifier::DiagnoseCommunication() {
    //CGlobal::Log("==== COMMUNICATION DIAGNOSIS ====");

    // 1. Check receiver state
    //CGlobal::Log("1. RECEIVER STATUS:");
    //CGlobal::Log("   - Receiver running: %s", _receiver.is_running() ? "YES" : "NO");

    // 2. Check thread state
    //CGlobal::Log("2. THREAD STATUS:");
    //CGlobal::Log("   - Termination event: %p", _terminateEvent);

    // 3. Check callback state
    //CGlobal::Log("3. CALLBACK STATUS:");
    //CGlobal::Log("   - Callback pointer: %p", _callback);
    //CGlobal::Log("   - Processing flag: %s", _processingCallback ? "YES" : "NO");
    //CGlobal::Log("   - Use queue: %s", _useQueue ? "YES" : "NO");

    // 4. Check queue state
    //CGlobal::Log("4. QUEUE STATUS:");
    //CGlobal::Log("   - Queue size: %d", _messageQueue.Size());

    // 5. Check tokens and connection
    //CGlobal::Log("5. CONNECTION STATUS:");
    //CGlobal::Log("   - Game ID: '%s'", _gameId);
    //CGlobal::Log("   - Game token length: %d", strlen(_gameToken));
    //CGlobal::Log("   - IsRunning: %s", IsRunning() ? "YES" : "NO");

    //CGlobal::Log("==================================");
}

void Notifier::SendError(int code, const wchar_t* message) {
    WCHAR errorBuffer[256];
    int len = swprintf_s(errorBuffer, 256, L"{\"status\":\"%d\", \"description\":\"%s\"}", code, message);
    EnterCriticalSection(&_callbackCS);
    if (_callback != NULL)
        _callback(Command::Error, errorBuffer, len * 2);
    LeaveCriticalSection(&_callbackCS);
}

bool Notifier::SendGeneral(Command command, const char* msg) {
    char utf8[512];
    int len = sprintf_s(utf8, 512, "%d|%s|%s|%s", command, NOTIFY_VERSION, _gameId, msg);

    //CGlobal::Log("Sending message to launcher: cmd=0x%08X, len=%d", command, len);
    //CGlobal::Log("Message content: %s", utf8);

    bool ret = _sender.sendUtf8String(PIPE_NAME, utf8, len);

    //CGlobal::Log("Send result: %s", ret ? "SUCCESS" : "FAILED");

    return ret;
}

bool Notifier::SendSecure(Command command, const char* msg) {
    if (msg == NULL || strlen(msg) == 0) {
        //CGlobal::Log("SendSecure: Invalid message");
        return false;
    }

    int len = strlen(msg);
    // Crear una copia del mensaje para no corromper el original
    char* msgCopy = new char[len + 1];
    strcpy_s(msgCopy, len + 1, msg);

    Encrypt((byte*)msgCopy, len);
    std::string msg1 = base64_encode((byte*)msgCopy, len);

    delete[] msgCopy; // Limpiar la copia

    return SendGeneral(command, msg1.c_str());
}

int Notifier::PullMessage() {
    int ret = 0;
    if (_useQueue) {
        SimpleMessage* msg = _messageQueue.Pop();
        if (msg) {
            //CGlobal::Log("PullMessage: Found message in queue, remaining: %d", _messageQueue.Size());

            EnterCriticalSection(&_callbackCS);
            if (_callback != NULL) {
                //CGlobal::Log("PullMessage: Calling callback for command 0x%08X", msg->command);
                _callback(msg->command, msg->data, msg->dataLen);
            } else {
                //CGlobal::Log("PullMessage: Callback is NULL - message discarded");
            }
            LeaveCriticalSection(&_callbackCS);

            delete msg;
            ret = _messageQueue.Size();
            //CGlobal::Log("PullMessage: Remaining messages in queue: %d", ret);
        } else {
            // No hay mensajes en la cola - esto es normal
            //CGlobal::Log("PullMessage: No messages in queue");
        }
    } else {
        //CGlobal::Log("PullMessage: Queue mode disabled");
    }
    return ret;
}

void Notifier::_process_message(byte* data, size_t len) {
    // CRITICAL: Check shutdown flag immediately
    if (_isShuttingDown) {
        //CGlobal::Log("*** SHUTDOWN *** Message processing skipped due to shutdown flag");
        return;
    }

    //CGlobal::Log("=== PROCESS_MESSAGE START ===");
    //CGlobal::Log("Processing message, length: %d", len);
    //CGlobal::Log("Raw data (first 100 chars): %.100s", (char*)data);

    Command command;
    byte *buffer = new byte[len];
    memcpy(&command, data, 4);

    //CGlobal::Log("Extracted command: 0x%08X", command);

    int decodedLen = base64_decode_to_buff_len((char*)(data + 4), len > 4 ? len - 4 : 0, buffer);
    //CGlobal::Log("Base64 decoded length: %d", decodedLen);

    if (decodedLen <= 0) {
        //CGlobal::Log("*** ERROR *** Base64 decode failed or empty result");
        delete[] buffer;
        return;
    }

    //CGlobal::Log("Before decrypt (first 50 bytes): %.50s", (char*)buffer);
    Decrypt(buffer, decodedLen);
    //CGlobal::Log("After decrypt (first 50 bytes): %.50s", (char*)buffer);

    // Asegurarse de que el buffer termina en null
    ((char*)buffer)[decodedLen] = 0;

    // Calcular el tamaño necesario para el buffer Unicode
    int wcharLen = MultiByteToWideChar(CP_UTF8, 0, (char*)buffer, decodedLen, NULL, 0);
    if (wcharLen <= 0) {
        //CGlobal::Log("*** ERROR *** Failed to calculate Unicode buffer size (Error: %d)", GetLastError());
        delete[] buffer;
        return;
    }

    // Crear el buffer del tamaño correcto
    wchar_t* message = new wchar_t[wcharLen + 1];  // +1 para el null terminator

    // Hacer la conversión real
    int result = MultiByteToWideChar(CP_UTF8, 0, (char*)buffer, decodedLen, message, wcharLen);
    if (result <= 0) {
        //CGlobal::Log("*** ERROR *** Failed to convert to Unicode (Error: %d)", GetLastError());
        delete[] message;
        delete[] buffer;
        return;
    }

    // Asegurar terminación null
    message[wcharLen] = 0;

    // Para el callback, la longitud es en bytes
    wcharLen = wcharLen * sizeof(wchar_t);

    //CGlobal::Log("Converted to Unicode - Chars: %d, Bytes: %d", result, wcharLen);
    //CGlobal::Log("Message content: %S", message);
    //CGlobal::Log("Final message content: %S", message);

    // Check shutdown flag before callback
    if (!_isShuttingDown) {
        if (!_useQueue) {
            //CGlobal::Log("*** CALLING CALLBACK DIRECTLY (not using queue) ***");
            // Use safe callback without C++ objects
            _safeCallback(command, message, wcharLen);
        }
        else {
            //CGlobal::Log("*** ADDING TO QUEUE (using queue mode) ***");
            try {
                // Only add to queue if still running and not shutting down
                if (IsRunning() && !_isShuttingDown) {
                    //CGlobal::Log("Adding message to queue");
                    if (_messageQueue.Push(command, message, wcharLen)) {
                        //CGlobal::Log("Queue size now: %d", _messageQueue.Size());
                    } else {
                        //CGlobal::Log("Failed to add message to queue (queue full)");
                    }
                } else {
                    //CGlobal::Log("Not running or shutting down - message not queued");
                }
            }
            catch (...) {
                //CGlobal::Log("Exception while accessing message queue");
                // Don't add to queue if it fails
            }
        }
    } else {
        //CGlobal::Log("*** SHUTDOWN *** Message callback skipped due to shutdown flag");
    }

    delete[] message;
    delete[] buffer;  // Critical fix: prevent memory leak
    //CGlobal::Log("=== PROCESS_MESSAGE END ===");
}

// PipeServer callback handler: receives raw message, decodes, and dispatches
void Notifier::_onPipeMessage(TCHAR* data, size_t len) {
    // CRITICAL: Check shutdown flag immediately
    if (_isShuttingDown) {
        //CGlobal::Log("*** SHUTDOWN *** Pipe message skipped due to shutdown flag");
        return;
    }
    //CGlobal::Log("=== ON_PIPE_MESSAGE START ===");
    //CGlobal::Log("Raw message received, length: %d", (int)len);
    //CGlobal::Log("Raw data (first 100 chars): %.100s", (char*)data);

    // Decode message
    Command command;
    byte* buffer = new byte[len];
    memcpy(&command, data, 4);
    int decodedLen = base64_decode_to_buff_len((char*)(data + 4), len > 4 ? len - 4 : 0, buffer);
    //CGlobal::Log("Base64 decoded length: %d", decodedLen);
    if (decodedLen <= 0) {
        //CGlobal::Log("*** ERROR *** Base64 decode failed or empty result");
        delete[] buffer;
        return;
    }
    Decrypt(buffer, decodedLen);
    ((char*)buffer)[decodedLen] = 0;
    int wcharLen = MultiByteToWideChar(CP_UTF8, 0, (char*)buffer, decodedLen, NULL, 0);
    if (wcharLen <= 0) {
        //CGlobal::Log("*** ERROR *** Failed to calculate Unicode buffer size (Error: %d)", GetLastError());
        delete[] buffer;
        return;
    }
    wchar_t* message = new wchar_t[wcharLen + 1];
    int result = MultiByteToWideChar(CP_UTF8, 0, (char*)buffer, decodedLen, message, wcharLen);
    if (result <= 0) {
        //CGlobal::Log("*** ERROR *** Failed to convert to Unicode (Error: %d)", GetLastError());
        delete[] message;
        delete[] buffer;
        return;
    }
    message[wcharLen] = 0;
    wcharLen = wcharLen * sizeof(wchar_t);
    //CGlobal::Log("Decoded message: %S", message);

    // Dispatch: queue or callback
    if (_useQueue) {
        //CGlobal::Log("*** ADDING TO QUEUE (using queue mode) ***");
        if (IsRunning() && !_isShuttingDown) {
            if (_messageQueue.Push(command, message, wcharLen)) {
                //CGlobal::Log("Queue size now: %d", _messageQueue.Size());
            }
            else {
                //CGlobal::Log("Failed to add message to queue (queue full)");
            }
        }
        else {
            //CGlobal::Log("Not running or shutting down - message not queued");
        }
    }
    else {
        //CGlobal::Log("*** CALLING SAFE CALLBACK (not using queue) ***");
        _safeCallback(command, message, wcharLen);
    }
    delete[] message;
    delete[] buffer;
    //CGlobal::Log("=== ON_PIPE_MESSAGE END ===");
}

CommandMessage::CommandMessage(Command _command, const wchar_t* _data, int len) {
    command = _command;
    dataLen = len;
    if (_data && len > 0) {
        data = new wchar_t[len + 1];
        memcpy(data, _data, len * sizeof(wchar_t));
        data[len] = 0;
    } else {
        data = nullptr;
    }
}

CommandMessage::~CommandMessage() {
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
}

// Unity-safe callback implementation without C++ objects
void Notifier::_safeCallback(Command command, wchar_t* message, int messageLen) {
    // CRITICAL: Check shutdown flag immediately
    if (_isShuttingDown) {
        //CGlobal::Log("*** SHUTDOWN *** Callback skipped due to shutdown flag");
        return;
    }

    // This function intentionally avoids C++ objects to allow SEH usage
    //CGlobal::Log("=== SAFE_CALLBACK START ===");
    //CGlobal::Log("Command: 0x%08X, Message length: %d", command, messageLen);
    //CGlobal::Log("Message content: %S", message);

    // CRITICAL: Prevent stack overflow from recursive callbacks
    if (_processingCallback) {
        //CGlobal::Log("*** WARNING *** Recursive callback detected - blocking to prevent stack overflow");
        return;
    }

    // Thread-safe callback access
    NotifyCallBack currentCallback = nullptr;

    // Quick atomic-style read of callback
    currentCallback = _callback;

    //CGlobal::Log("Current callback pointer: %p", currentCallback);
    //CGlobal::Log("IsRunning status: %s", IsRunning() ? "TRUE" : "FALSE");

    // Simple flag-based check without mutex (safer for SEH)
    if (currentCallback == nullptr) {
        //CGlobal::Log("*** ERROR *** Callback is NULL - message will be discarded");
        return;
    }

    if (!IsRunning() || _isShuttingDown) {
        //CGlobal::Log("*** ERROR *** Not running or shutting down - message will be discarded");
        return;
    }

    _processingCallback = true;  // Set guard flag
    //CGlobal::Log("*** CALLING UNITY CALLBACK *** About to call Unity callback function");

    __try {
        currentCallback(command, message, messageLen);
        //CGlobal::Log("*** SUCCESS *** Unity callback completed successfully");
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        //CGlobal::Log("*** EXCEPTION *** Exception in callback - Unity may be shutting down (Code: 0x%08X)", GetExceptionCode());
        // Don't propagate exception - just log and continue
    }

    _processingCallback = false;  // Clear guard flag
    //CGlobal::Log("=== SAFE_CALLBACK END ===");
}
