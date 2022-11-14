#pragma once
#include "cmsis_os2.h"

void Error_Handler(void);

class Semaphore {
    osSemaphoreId_t handle = nullptr;

public:
    Semaphore() = default;
    Semaphore(Semaphore&&);
    ~Semaphore();

    Semaphore& operator=(Semaphore&&);

    // Semaphores Must be initialized from a thread, otherwise they will not
    // work properly.
    static Semaphore New(uint32_t max_count, uint32_t initial_count);


    bool isInitialized() const;

    bool release();
    bool acquire(uint32_t timeout = osWaitForever);
};
