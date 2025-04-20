#ifndef THREAD_H_
#define THREAD_H_

#include "pthread.h"
typedef void* (*ThreadFunction)(void* /*param*/);
typedef void* ThreadReturn;
typedef pthread_t ThreadId;

class Thread
{
private:
    ThreadId _threadId;

public:
    Thread();
    ~Thread();

    bool Start(ThreadFunction /*callback*/, void* /*param*/);
};

#endif