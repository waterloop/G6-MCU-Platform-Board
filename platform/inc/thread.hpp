#pragma once
#include "stdint.h"
#include "cmsis_os2.h"
#include <array>

enum ThreadPriority : uint8_t {
    Normal = 0,
    RealTime,
    MaxPriority,
    Idle,

    NumPriorities
};

void threadFunc(void *thread);

class Thread {
    ThreadPriority priority;

public:
    constexpr Thread(ThreadPriority priority)
        : priority(priority) {}
    virtual void Task() = 0;

    ThreadPriority get_priority() const;
    const osPriority_t get_os_priority() const;
protected:
    void yield();
};
