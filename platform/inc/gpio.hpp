#pragma once
#include "gpio.h"

template<uint32_t PORT, uint16_t PIN>
class GpioDriver {
public:
    GpioDriver() = default;

    void set() {
        HAL_GPIO_WritePin(PORT, PIN, GPIO_PIN_SET);
    }
    void reset() {
        HAL_GPIO_WritePin(PORT, PIN, GPIO_PIN_RESET);
    }
};

#ifdef DEBUG_LED_GPIO_Port
#ifdef DEBUG_LED_Pin
// static constexpr GPIO_TypeDef *DEBUG_LED_PORT = DEBUG_LED_GPIO_Port;
typedef GpioDriver<GPIOB_BASE, DEBUG_LED_Pin> DebugLedDriver;

#endif
#endif
