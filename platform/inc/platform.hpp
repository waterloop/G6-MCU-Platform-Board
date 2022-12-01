#pragma once
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
#include "can.hpp"
#include "cmsis_os.h"

class Thread;

void SystemClock_Config(void);

/**
 * @brief This is where the developer can register the threads
 * that their board will be using. See the default implementation
 * for examples.
 *
 */
void register_threads();

class Platform {
  void initialize_platform();
  Platform(Platform&) = delete;
  Platform(Platform&&) = delete;

public:
  Platform() = default;
  [[noreturn]] void run();

protected:
  /**
   * @brief add_threads
   * When implementing this function, declare static threads
   * in the function body, and then pass them into the add_thread
   * method
   */
  virtual void add_threads() = 0;

  void add_thread(Thread *thread);
};
