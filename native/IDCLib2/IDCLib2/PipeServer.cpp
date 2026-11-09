#include "PipeServer.h"
#include "CGlobal.h"

PipeServer::PipeServer() {
    //CGlobal::Log("PipeServer constructor called");
    _running = false;
    _thread = NULL;

    // Inicializar todos los handles a NULL
    memset(_pipes, 0, MAX_INSTANCES * sizeof(PIPEINST));
    memset(_events, 0, (MAX_INSTANCES + 1) * sizeof(HANDLE));

    // Inicializar handles específicamente
    for (int i = 0; i < MAX_INSTANCES; i++)
    {
        _pipes[i].hPipeInst = NULL;
        _pipes[i].chRequest = std::make_unique<SafeBuffer>(BUFF_SIZE);
        _pipes[i].chReply = std::make_unique<SafeBuffer>(BUFF_SIZE);
        _events[i] = NULL;
    }
    _events[MAX_INSTANCES] = NULL;

    // Inicializar critical section
    //CGlobal::Log("PipeServer constructor completed");
}
PipeServer::~PipeServer()
{
    //CGlobal::Log("PipeServer destructor called");

    // Primero asegurar que está parado
    if (_running) {
        stop();
        // Esperar un poco para que el thread termine
        Sleep(100);
    }

    // Limpiar critical section de forma segura

    // Cerrar handles de pipes solo si son válidos
    for (int i = 0; i < MAX_INSTANCES; i++)
    {
        if (_isValidHandle(_pipes[i].hPipeInst)) {
            // Asegurar que todos los datos pendientes se envíen
            FlushFileBuffers(_pipes[i].hPipeInst);
            // Esperar un momento para que se complete cualquier operación pendiente
            Sleep(1);
            // Desconectar y cerrar el pipe
            DisconnectNamedPipe(_pipes[i].hPipeInst);
            CloseHandle(_pipes[i].hPipeInst);
            _pipes[i].hPipeInst = NULL;
        }

        if (_isValidHandle(_events[i])) {
            CloseHandle(_events[i]);
            _events[i] = NULL;
        }

        // Los SafeBuffer se limpian automáticamente por RAII
        _pipes[i].chRequest.reset();
        _pipes[i].chReply.reset();
    }

    // Cerrar evento de control solo si es válido
    if (_isValidHandle(_events[MAX_INSTANCES])) {
        CloseHandle(_events[MAX_INSTANCES]);
        _events[MAX_INSTANCES] = NULL;
    }

    //CGlobal::Log("PipeServer destructor completed");
}

bool PipeServer::is_running() {
    return _running;
}

bool PipeServer::start(const char *pipeName)
{
    //CGlobal::Log("PipeServer starting with pipe: %s", pipeName);

    for (int i = 0; i < MAX_INSTANCES; i++)
    {
        // Create an event object for this instance.
        _events[i] = CreateEvent(NULL, TRUE, TRUE, NULL);
        if (_events[i] == NULL) {
            //CGlobal::Log("Failed to create event for instance %d", i);
            // Limpiar eventos ya creados
            for (int j = 0; j < i; j++) {
                if (_events[j] != NULL) {
                    CloseHandle(_events[j]);
                    _events[j] = NULL;
                }
            }
            return FALSE;
        }

        _pipes[i].oOverlap.hEvent = _events[i];
        _pipes[i].hPipeInst = CreateNamedPipe(
            pipeName,            // pipe name
            PIPE_ACCESS_DUPLEX |     // read/write access
            FILE_FLAG_OVERLAPPED,    // overlapped mode
            PIPE_TYPE_MESSAGE |      // message-type pipe
            PIPE_READMODE_MESSAGE |  // message-read mode
            PIPE_WAIT,               // blocking mode
            MAX_INSTANCES,               // number of MAX_INSTANCES
            BUFF_SIZE * sizeof(TCHAR),   // output buffer size
            BUFF_SIZE * sizeof(TCHAR),   // input buffer size
            PIPE_TIMEOUT,            // client time-out
            NULL);                   // default security attributes

        if (_pipes[i].hPipeInst == INVALID_HANDLE_VALUE)
        {
            //CGlobal::Log("Failed to create named pipe for instance %d, error: %d", i, GetLastError());
            // Limpiar recursos ya creados
            for (int j = 0; j <= i; j++) {
                if (_events[j] != NULL) {
                    CloseHandle(_events[j]);
                    _events[j] = NULL;
                }
                if (j < i && _pipes[j].hPipeInst != NULL && _pipes[j].hPipeInst != INVALID_HANDLE_VALUE) {
                    CloseHandle(_pipes[j].hPipeInst);
                    _pipes[j].hPipeInst = NULL;
                }
            }
            return false;
        }

        // Call the subroutine to connect to the new client
        _pipes[i].fPendingIO = _connectToNewClient(_pipes[i].hPipeInst, &_pipes[i].oOverlap);
        _pipes[i].dwState = _pipes[i].fPendingIO ? CONNECTING_STATE : READING_STATE;
    }
    _events[MAX_INSTANCES] = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (_events[MAX_INSTANCES] == NULL) {
        //CGlobal::Log("Failed to create control event");
        // Limpiar todo si falla la creación del evento de control
        for (int i = 0; i < MAX_INSTANCES; i++) {
            if (_events[i] != NULL) {
                CloseHandle(_events[i]);
                _events[i] = NULL;
            }
            if (_pipes[i].hPipeInst != NULL && _pipes[i].hPipeInst != INVALID_HANDLE_VALUE) {
                CloseHandle(_pipes[i].hPipeInst);
                _pipes[i].hPipeInst = NULL;
            }
        }
        return false;
    }

    _running = true;
    _thread = CreateThread(NULL, 0, _threadProc, this, 0, NULL);
    if (_thread == NULL) {
        //CGlobal::Log("Failed to create PipeServer thread");
        _running = false;
        return false;
    }

    //CGlobal::Log("PipeServer started successfully");
    return true;
}

void PipeServer::stop() {
    if (!_running) return;

    //CGlobal::Log("PipeServer stopping...");
    _running = false;

    // Señalar al thread que termine
    if (_isValidHandle(_events[MAX_INSTANCES])) {
        PulseEvent(_events[MAX_INSTANCES]);
    }

    // Esperar a que el thread termine
    if (_isValidHandle(_thread)) {
        DWORD waitResult = WaitForSingleObject(_thread, 2000);
        if (waitResult == WAIT_TIMEOUT) {
            //CGlobal::Log("PipeServer thread did not terminate gracefully");
            TerminateThread(_thread, 0);
        }
        CloseHandle(_thread);
        _thread = NULL;
    }

    // Limpiar los pipes de forma segura
    for (int i = 0; i < MAX_INSTANCES; i++) {
        if (_isValidHandle(_pipes[i].hPipeInst)) {
            // Asegurar que todos los datos se han enviado
            FlushFileBuffers(_pipes[i].hPipeInst);
            // Desconectar el pipe
            DisconnectNamedPipe(_pipes[i].hPipeInst);
            // Cerrar el handle
            CloseHandle(_pipes[i].hPipeInst);
            _pipes[i].hPipeInst = NULL;
        }

        if (_isValidHandle(_events[i])) {
            CloseHandle(_events[i]);
            _events[i] = NULL;
        }
    }

    //CGlobal::Log("PipeServer stopped and pipes cleaned up");
}

bool PipeServer::_isValidHandle(HANDLE handle) {
    return (handle != NULL && handle != INVALID_HANDLE_VALUE);
}

// DisconnectAndReconnect(DWORD)
// This function is called when an error occurs or when the client
// closes its handle to the pipe. Disconnect from this client, then
// call ConnectNamedPipe to wait for another client to connect.

void PipeServer::_disconnectAndReconnect(DWORD i)
{
    // Disconnect the pipe instance.

    if (!DisconnectNamedPipe(_pipes[i].hPipeInst))
    {
        //printf("DisconnectNamedPipe failed with %d.\n", GetLastError());
    }

    // Call a subroutine to connect to the new client.

    _pipes[i].fPendingIO = _connectToNewClient(
        _pipes[i].hPipeInst,
        &_pipes[i].oOverlap);

    _pipes[i].dwState = _pipes[i].fPendingIO ?
        CONNECTING_STATE : // still connecting
        READING_STATE;     // ready to read
}

// ConnectToNewClient(HANDLE, LPOVERLAPPED)
// This function is called to start an overlapped connect operation.
// It returns TRUE if an operation is pending or FALSE if the
// connection has been completed.

bool PipeServer::_connectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo)
{
    bool fConnected, fPendingIO = false;

    // Start an overlapped connection for this pipe instance.
    fConnected = ConnectNamedPipe(hPipe, lpo);

    // Overlapped ConnectNamedPipe should return zero.
    if (fConnected)
    {
        //printf("ConnectNamedPipe failed with %d.\n", GetLastError());
        return 0;
    }

    switch (GetLastError())
    {
        // The overlapped connection in progress.
    case ERROR_IO_PENDING:
        fPendingIO = true;
        break;

        // Client is already connected, so signal an event.

    case ERROR_PIPE_CONNECTED:
        if (SetEvent(lpo->hEvent))
            break;

        // If an error occurs during the connect operation...
    default:
    {
        //printf("ConnectNamedPipe failed with %d.\n", GetLastError());
        return 0;
    }
    }

    return fPendingIO;
}

void PipeServer::_getAnswerToRequest(LPPIPEINST pipe)
{
    _tprintf(TEXT("[%d] %s\n"), pipe->hPipeInst, pipe->chRequest->get());
    //StringCchCopy(pipe->chReply, BUFSIZE, TEXT("Default answer from server"));
    //pipe->cbToWrite = (lstrlen(pipe->chReply) + 1) * sizeof(TCHAR);
}

void PipeServer::setMessageCallback(void (*callback)(TCHAR* data, size_t len)) {
    _onMessageCallback = callback;
}

void PipeServer::_onNewMessage(TCHAR* data, size_t len)
{
    //CGlobal::Log("*** PipeServer::_onNewMessage CALLED *** LENGTH: %d", len);
    //CGlobal::Log("*** RAW DATA *** (first 100 chars): %.100s", data);
    if (_onMessageCallback) {
        _onMessageCallback(data, len);
        //CGlobal::Log("*** CALLBACK EXECUTED ***");
    } else {
        //CGlobal::Log("*** WARNING: Callback not set, message discarded ***");
    }
    //CGlobal::Log("*** PipeServer::_onNewMessage COMPLETED ***");
}


DWORD PipeServer::_threadProc(LPVOID param)
{
    PipeServer* pThis = (PipeServer*)param;
    DWORD dwWait, cbRet, dwErr;
    BOOL fSuccess;
    while (pThis->_running) {
        dwWait = WaitForMultipleObjects(
            MAX_INSTANCES+1,    // number of event objects
            pThis->_events,      // array of event objects
            FALSE,        // does not wait for all
            INFINITE);    // waits indefinitely

      // dwWait shows which pipe completed the operation.
        if (dwWait > MAX_INSTANCES-1)
            break;

        int i = dwWait;
        // Get the result if the operation was pending.
        if (pThis->_pipes[i].fPendingIO)
        {
            fSuccess = GetOverlappedResult(
                pThis->_pipes[i].hPipeInst, // handle to pipe
                &pThis->_pipes[i].oOverlap, // OVERLAPPED structure
                &cbRet,            // bytes transferred
                FALSE);            // do not wait

            switch (pThis->_pipes[i].dwState)
            {
                // Pending connect operation
            case CONNECTING_STATE:
                if (!fSuccess)
                {
                    //printf("Error %d.\n", GetLastError());
                    return 0;
                }
                pThis->_pipes[i].dwState = READING_STATE;
                break;

                // Pending read operation
            case READING_STATE:
                if (!fSuccess || cbRet == 0)
                {
                    pThis->_disconnectAndReconnect(i);
                    continue;
                }
                pThis->_pipes[i].cbRead = cbRet;
                pThis->_pipes[i].dwState = WRITING_STATE;
                break;

                // Pending write operation
            case WRITING_STATE:
                if (!fSuccess || cbRet != pThis->_pipes[i].cbToWrite)
                {
                    pThis->_disconnectAndReconnect(i);
                    continue;
                }
                pThis->_pipes[i].dwState = READING_STATE;
                break;

            default:
            {
                //printf("Invalid pipe state.\n");
                return 0;
            }
            }
        }

        // The pipe state determines which operation to do next.

        switch (pThis->_pipes[i].dwState)
        {
            // READING_STATE:
            // The pipe instance is connected to the client
            // and is ready to read a request from the client.

            case READING_STATE:
                // Pequeño delay para dar tiempo a que el cliente escriba
                Sleep(1);
                fSuccess = ReadFile(
                    pThis->_pipes[i].hPipeInst,
                    pThis->_pipes[i].chRequest->get(),
                    BUFF_SIZE * sizeof(TCHAR),
                    &pThis->_pipes[i].cbRead,
                    &pThis->_pipes[i].oOverlap);

                // The read operation completed successfully.

                if (fSuccess && pThis->_pipes[i].cbRead != 0)
                {
                    TCHAR* buf = pThis->_pipes[i].chRequest->get();
                    if (pThis->_pipes[i].cbRead < BUFF_SIZE) {
                        buf[pThis->_pipes[i].cbRead] = 0;
                    }
                    pThis->_onNewMessage(buf, pThis->_pipes[i].cbRead);
                    pThis->_pipes[i].fPendingIO = FALSE;
                    pThis->_pipes[i].dwState = WRITING_STATE;
                    continue;
                }

                // The read operation is still pending.

                dwErr = GetLastError();
                if (!fSuccess && (dwErr == ERROR_IO_PENDING))
                {
                    pThis->_pipes[i].fPendingIO = TRUE;
                    continue;
                }

                // An error occurred; disconnect from the client.

                pThis->_disconnectAndReconnect(i);
                break;

                // WRITING_STATE:
                // The request was successfully read from the client.
                // Get the reply data and write it to the client.

            case WRITING_STATE:
                //pThis->_getAnswerToRequest(&pThis->_pipes[i]);
                //fSuccess = WriteFile(
                //    pThis->_pipes[i].hPipeInst,
                //    pThis->_pipes[i].chReply,
                //    pThis->_pipes[i].cbToWrite,
                //    &cbRet,
                //    &pThis->_pipes[i].oOverlap);
                // The write operation completed successfully.
                if (fSuccess && cbRet == pThis->_pipes[i].cbToWrite)
                {
                    pThis->_pipes[i].fPendingIO = FALSE;
                    pThis->_pipes[i].dwState = READING_STATE;
                    continue;
                }

                // The write operation is still pending.

                dwErr = GetLastError();
                if (!fSuccess && (dwErr == ERROR_IO_PENDING))
                {
                    pThis->_pipes[i].fPendingIO = TRUE;
                    continue;
                }

                // An error occurred; disconnect from the client.

                pThis->_disconnectAndReconnect(i);
                break;

            default:
            {
                //printf("Invalid pipe state.\n");
                return 0;
            }
        }
    }

    //CGlobal::Log("PipeServer thread ending, cleaning up handles...");

    // Limpiar handles solo si son válidos
    for (int i = 0; i < MAX_INSTANCES; i++) {
        if (pThis->_isValidHandle(pThis->_pipes[i].hPipeInst)) {
            DisconnectNamedPipe(pThis->_pipes[i].hPipeInst);
            CloseHandle(pThis->_pipes[i].hPipeInst);
            pThis->_pipes[i].hPipeInst = NULL;
        }
        if (pThis->_isValidHandle(pThis->_events[i])) {
            CloseHandle(pThis->_events[i]);
            pThis->_events[i] = NULL;
        }
    }

    if (pThis->_isValidHandle(pThis->_events[MAX_INSTANCES])) {
        CloseHandle(pThis->_events[MAX_INSTANCES]);
        pThis->_events[MAX_INSTANCES] = NULL;
    }

    pThis->_running = false;
    // No establecer _thread = NULL aquí, lo hace stop()

    //CGlobal::Log("PipeServer thread cleanup completed");
    return 1;
}
