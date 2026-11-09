#pragma once

#include <queue>
#include <mutex>

typedef void(__stdcall* LogCallback)(const char* string, int len);

class CLogger {
    private:
        bool                        started;
        std::queue<std::string*>    queue;
        std::mutex                  mutex;
        std::mutex                  callbackMutex;
        LogCallback                 callback;
        bool                        useQueue;
    public:
        CLogger();
        void Start(LogCallback _callback, bool _useQueue);
        void Stop();
        void Log(const char* message, int len);

        int PullMessage();

};
