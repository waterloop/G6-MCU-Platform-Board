/**
 * @file platform.hpp
 * @author Waterloop
 * @brief The primary header for the G6 Mcu platform board
 * @version 0.1
 * @date 2022-10-31
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef   __WEAK
  #define __WEAK                                 __attribute__((weak))
#endif
#include "can.hpp"
#include "cmsis_os.h"

void SystemClock_Config(void);
void MX_FREERTOS_Init(void);

/**
 * @brief This is where the developer can register the threads
 * that their board will be using. See the default implementation
 * for examples.
 *
 */
void register_threads();
