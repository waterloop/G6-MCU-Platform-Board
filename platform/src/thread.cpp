#include "thread.hpp"

Thread::Thread(ThreadPriority priority)
    : priority(priority) {}

void threadFunc(void *thread) {
    ((Thread*)thread)->Task();
}

void Thread::yield() {
    osThreadYield();
}
