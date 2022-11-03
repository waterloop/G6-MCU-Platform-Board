#include "platform.hpp"


__WEAK void register_threads() {

}

int main() {
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    SystemClock_Config();

    osKernelInitialize();  /* Call init function for freertos objects (in freertos.c) */
    MX_FREERTOS_Init();

    CanDriver can_driver = CanDriver::get_driver();
    can_driver.initialize();

    register_threads();

    osKernelStart();

    while(1); // ONLY HIT HERE IF ERROR
}
