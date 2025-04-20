#include "thread.h"

Thread::Thread() : _threadId(0) {}

Thread::~Thread() {}

bool Thread::Start(ThreadFunction callback, void* param)
{
    if (pthread_create(&_threadId, NULL, *callback, param) == 0) {
        return true;
    }
    return false;
}