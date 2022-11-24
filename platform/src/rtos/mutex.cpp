#include "rtos/mutex.hpp"

Mutex::~Mutex() {
    if (handle != nullptr) {
        osMutexDelete(handle);
    }
}

Mutex::Mutex(Mutex&& other)
    : handle(other.handle) {
    other.handle = nullptr;
}

Mutex& Mutex::operator=(Mutex&& other) {
    handle = other.handle;
    other.handle = nullptr;
    return *this;
}

Mutex Mutex::New() {
    auto tid = osThreadGetId();
    // Ensure we are in a thread when creating
    // Eliminates a bug where the Mutex Size can get set to 0 when the os starts
    if (tid == NULL) {
        Error_Handler();
    }
    Mutex s;
    s.handle = osMutexNew(NULL);
    return s;
}

bool Mutex::isInitialized() const {
    return handle != nullptr;
}

bool Mutex::release() {
    auto result = osMutexRelease(handle);
    return result == osOK;
}

bool Mutex::acquire(uint32_t timeout) {
    auto result = osMutexAcquire(handle, timeout);
    return result == osOK;
}

bool Mutex::criticalSection(std::function<void()> f_criticalSection) {
    if (!isInitialized()) { return false; }
    if (!acquire()) { return false; }
    f_criticalSection();
    if (!release()) { return false; }
    return true;
}
