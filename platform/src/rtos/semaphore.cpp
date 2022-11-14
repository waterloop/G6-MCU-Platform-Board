#include "rtos/semaphore.hpp"

Semaphore::~Semaphore() {
    if (handle != nullptr) {
        osSemaphoreDelete(handle);
    }
}

Semaphore::Semaphore(Semaphore&& other)
    : handle(other.handle) {
    other.handle = nullptr;
}

Semaphore& Semaphore::operator=(Semaphore&& other) {
    handle = other.handle;
    other.handle = nullptr;
}

Semaphore Semaphore::New(uint32_t max_count, uint32_t initial_count) {
    auto tid = osThreadGetId();
    // Ensure we are in a thread when creating
    // Eliminates a bug where the Semaphore Size can get set to 0 when the os starts
    if (tid == NULL) {
        Error_Handler();
    }
    Semaphore s;
    s.handle = osSemaphoreNew(max_count, initial_count, NULL);
    return s;
}

bool Semaphore::isInitialized() const {
    return handle != nullptr;
}

bool Semaphore::release() {
    auto result = osSemaphoreRelease(handle);
    return result == osOK;
}

bool Semaphore::acquire(uint32_t timeout) {
    auto result = osSemaphoreAcquire(handle, timeout);
    return result == osOK;
}
