#include "CLogger.h"
#include "CGlobal.h"

CLogger::CLogger() {
    callback = NULL;
    useQueue = false;
    started = false;
}

void CLogger::Start(LogCallback _callback, bool _useQueue) {
    callbackMutex.lock();
    callback = _callback;
    callbackMutex.unlock();
    started = true;
    useQueue = _useQueue;

    mutex.lock();
    int size = queue.size();
    mutex.unlock();
    //CGlobal::Log("CLogger::Start queue size: %d", size);
}

void CLogger::Stop() {
    started = false;
    callbackMutex.lock();
    callback = NULL;
    callbackMutex.unlock();
}

int CLogger::PullMessage() {
    if (!started || !useQueue) return 0;
    int ret = 0;
    mutex.lock();
    if(!queue.empty()) {
        std::string* entry = queue.front();
        queue.pop();
        callbackMutex.lock();
        if(callback!=NULL) callback(entry->c_str(), entry->length());
        callbackMutex.unlock();
        delete entry;
        ret = queue.size();
    }
    mutex.unlock();
return ret;
}

void CLogger::Log(const char* _message, int len) {
    if (useQueue) {
        mutex.lock();
        queue.push(new std::string(_message, len));
        mutex.unlock();
    }
    else {
        callbackMutex.lock();
        if (callback != NULL) callback(_message, strlen(_message));
        callbackMutex.unlock();
    }
}
