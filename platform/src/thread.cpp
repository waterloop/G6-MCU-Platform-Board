#include "thread.hpp"
#include "platform.hpp"

static constexpr std::array<osPriority_t, ThreadPriority::NumPriorities> g_rtosPrioLookup = {
    osPriorityNormal,
    osPriorityRealtime,
    osPriorityRealtime7,
    osPriorityIdle,
};

void threadFunc(void *thread) {
    ((Thread*)thread)->Task();
}

void Thread::yield() {
    osThreadYield();
}

ThreadPriority Thread::get_priority() const {
    return priority;
}

const osPriority_t Thread::get_os_priority() const {
    return g_rtosPrioLookup[priority];
}
