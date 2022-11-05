#pragma once
#include "platform.hpp"

enum ThreadPriority {
    Normal,
    RealTime,
};

void threadFunc(void *thread);

class Thread {
    ThreadPriority priority;

public:
    Thread(ThreadPriority priority);
    virtual void Task() = 0;


protected:
    void yield();
};
