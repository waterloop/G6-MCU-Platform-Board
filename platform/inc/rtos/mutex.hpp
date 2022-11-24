#pragma once
#include "main.h"
#include "cmsis_os2.h"
#include <functional>

class Mutex {
    osMutexId_t handle = nullptr;

    bool release();
    bool acquire(uint32_t timeout = osWaitForever);
    bool isInitialized() const;

public:
    Mutex() = default;
    Mutex(Mutex&&);
    ~Mutex();

    Mutex& operator=(Mutex&&);

    // Mutexes Must be initialized from a thread, otherwise they will not
    // work properly.
    static Mutex New();

    /* ensures that acquire and release are called in the proper order and checks their return value */
    bool criticalSection(std::function<void()> f_criticalSection);
};
