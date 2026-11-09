#include "CGlobal.h"
#include "CLogger.h"
#include <shlobj_core.h>
#include <algorithm>

HMODULE CGlobal::g_hInstance = NULL;
std::unique_ptr<CGlobal> CGlobal::instance_ = nullptr;
std::once_flag CGlobal::initialized_;
std::mutex CGlobal::log_mutex_;

CGlobal* CGlobal::Instance() {
    std::call_once(initialized_, []() {
        instance_ = std::unique_ptr<CGlobal>(new CGlobal());
        });
    return instance_.get();
}

CGlobal::CGlobal() {
    // Create logger first to avoid recursion in Log() calls
    m_logger = new CLogger();

    // Now safe to create notifier (which may call Log())
    m_notifier = new Notifier();
}


CGlobal::~CGlobal() {
    delete m_notifier;
    m_notifier = NULL;
    delete m_logger;
    m_logger = NULL;
}

void CGlobal::LogOld(const char* _Format, ...) {
    // Thread-safe logging with proper synchronization
    std::lock_guard<std::mutex> lock(log_mutex_);

    char buff[512];
    int len;

    // Check if the format string contains any format specifiers
    bool hasFormatSpecifiers = strchr(_Format, '%') != nullptr;

    if (hasFormatSpecifiers) {
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        len = _vsprintf_s_l(buff, 512, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
    }
    else {
        // If no format specifiers, just copy the string
        len = strlen(_Format);
        if (len >= 512) len = 511;
        memcpy(buff, _Format, len);
        buff[len] = '\0';
    }
    // Safe access to instance
    CGlobal* instance = Instance();
    if (instance != nullptr && instance->m_logger != nullptr)
        instance->m_logger->Log(buff, len);

    // Activar logging a archivo para debugging
    try {
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);

        // Extract the directory path
        char* lastSlash = strrchr(exePath, '\\');
        if (lastSlash != NULL) {
            *(lastSlash + 1) = '\0';  // Truncate after the last slash
        }

        // Construct the log file path in the executable's directory
        char logPath[MAX_PATH];
        sprintf_s(logPath, "%sIDCLib2.log", exePath);

        // Open for append
        FILE* fp = nullptr;
        errno_t err = fopen_s(&fp, logPath, "a");

        if (fp != nullptr) {
            // Get timestamp
            SYSTEMTIME st;
            GetLocalTime(&st);

            // Write the log entry
            fprintf(fp, "[%02d:%02d:%02d.%03d] %s\n",
                st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, buff);

            fclose(fp);
        }
    }
    catch (...) {
        // If file logging fails, continue without it
    }
}

/*
void CGlobal::Log(const char* _Format, ...) {
    // Get the instance first to ensure logger exists
    CGlobal* instance = Instance();
    if (instance == nullptr || instance->m_logger == nullptr) {
        return;  // Cannot log without logger
    }

    try {
        char buff[512];
        va_list args;
        va_start(args, _Format);

        // Use vsnprintf for a safer implementation
        #if defined(_MSC_VER)
            int len = vsnprintf_s(buff, sizeof(buff), _TRUNCATE, _Format, args);
        #else
            int len = vsnprintf(buff, sizeof(buff), _Format, args);
        #endif

        va_end(args);

        if (len < 0) {
            return;  // Format error
        }
        // Ensure null termination and proper length
        if (len >= sizeof(buff)) {
            len = sizeof(buff) - 1;
            buff[len] = '\0';
        }

        // Simple synchronization - one mutex for all operations
        std::lock_guard<std::mutex> lock(log_mutex_);

        // Log to logger
        instance->m_logger->Log(buff, len);
    }
    catch (...) {
        // If anything fails, we fail silently
    }
}
    */

void CGlobal::Log(const char* _Format, ...)
{
    try {
        char buff[512];
        va_list args;
        va_start(args, _Format);

#if defined(_MSC_VER)
        int len = vsnprintf_s(buff, sizeof(buff), _TRUNCATE, _Format, args);
#else
        int len = vsnprintf(buff, sizeof(buff), _Format, args);
#endif

        va_end(args);

        if (len < 0) {
            return; // Error en el formato
        }

        if (len >= sizeof(buff)) {
            len = sizeof(buff) - 1;
            buff[len] = '\0';
        }

        // Sincronización para que varios hilos no escriban a la vez
        std::lock_guard<std::mutex> lock(log_mutex_);

        // Construir la ruta del log en el mismo directorio del ejecutable
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        char* lastSlash = strrchr(exePath, '\\');
        if (lastSlash != NULL) {
            *(lastSlash + 1) = '\0';  // Truncar después del último slash
        }

        char logPath[MAX_PATH];
        //sprintf_s(logPath, "%sIDCLib2.log", exePath);
        sprintf_s(logPath, "c:\\temp\\IDCLib2.log");

        // Abrir para append
        FILE* fp = nullptr;
        errno_t err = fopen_s(&fp, logPath, "a");
        if (fp == nullptr || err != 0) {
            return; // no se pudo abrir
        }

        // Timestamp
        SYSTEMTIME st;
        GetLocalTime(&st);

        fprintf(fp, "[%02d:%02d:%02d.%03d] %s\n",
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, buff);

        fclose(fp);
    }
    catch (...) {
        // Si falla, ignoramos para no romper la app
    }
}
