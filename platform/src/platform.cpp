#include "platform.hpp"
#include "thread.hpp"
#include "gpio.hpp"

void Platform::initialize_platform() {
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    SystemClock_Config();
    MX_GPIO_Init();

    osKernelInitialize();  /* Call init function for freertos objects (in freertos.c) */

    CanDriver can_driver = CanDriver::get_driver();
    can_driver.initialize();

}

void Platform::run() {
    initialize_platform();
    add_threads();
    osKernelStart();
    while(1);
}

void Platform::add_thread(Thread *thread) {
    osThreadNew(threadFunc, thread, nullptr);
}
