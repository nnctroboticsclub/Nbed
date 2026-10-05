#if defined(STM32F446xx)

#include "f4/system.h"

#include <sys/stat.h>
#include <sys/times.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>

#include "stm32f4xx_hal.h"

#include "logger.h"

#include "f4/clock.h"
#include "f4/dma.h"
#include "f4/time.h"
#include "f4/uart.h"

#ifndef NBED_NO_DEFAULT_SERIAL
#include "interface/pin_names.h"
#endif  // NBED_NO_DEFAULT_SERIAL

namespace nbed::f4 {

void Error::ErrorHandler() {
  __disable_irq();
  Logger::Error("SystemError", "An error has occurred.");
  while (true) {}
}

void System::Initialize() {
  HAL_Init();
  if (!ClockManager::Initialize()) {
    ErrorHandler();
  }
  Time::Initialize();
  DmaManager::Initialize();

#ifndef NBED_NO_DEFAULT_SERIAL
  static Uart uart(UartPeripheral::kUsart2, interface::PinName::kPA3,
                   interface::PinName::kPA2, 921600);
  if (uart.Initialize()) {
    uart.SetConsole();
    Logger::Info("System",
                 "Default serial console initialized on USART2 (PA3/PA2)");
  }
#endif  // NBED_NO_DEFAULT_SERIAL
}

void System::Initialize(const Clock::Configuration& configuration) {
  HAL_Init();
  if (!ClockManager::Initialize(configuration)) {
    ErrorHandler();
  }
  Time::Initialize();
  DmaManager::Initialize();

#ifndef NBED_NO_DEFAULT_SERIAL
  static Uart uart(UartPeripheral::kUsart2, interface::PinName::kPA3,
                   interface::PinName::kPA2, 921600);
  if (uart.Initialize()) {
    uart.SetConsole();
    Logger::Info("System",
                 "Default serial console initialized on USART2 (PA3/PA2)");
  }
#endif  // NBED_NO_DEFAULT_SERIAL
}

void System::ErrorHandler() {
  static Error error;
  error.ErrorHandler();
}

extern "C" {

// ===== Hal Msp =====
void HAL_MspInit(void) {
  __HAL_RCC_SYSCFG_CLK_ENABLE();  // NOLINT
  __HAL_RCC_PWR_CLK_ENABLE();     // NOLINT
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_0);
}

// ===== System Interrupt Handler =====
void NMI_Handler(void) {  // NOLINT
  while (true) {}
}

void HardFault_Handler(void) {  // NOLINT
  while (true) {}
}

void MemManage_Handler(void) {  // NOLINT
  while (true) {}
}

void BusFault_Handler(void) {  // NOLINT
  while (true) {}
}

void UsageFault_Handler(void) {  // NOLINT
  while (true) {}
}

void SVC_Handler(void) {  // NOLINT
}

void DebugMon_Handler(void) {  // NOLINT
}

void PendSV_Handler(void) {  // NOLINT
}

// ===== Error Handler =====
void Error_Handler(void) {  // NOLINT
  System::ErrorHandler();
}

// ===== System Calls =====
__attribute__((weak)) int __io_putchar(int ch) {  // NOLINT
  (void)ch;
  return -1;
}

__attribute__((weak)) int __io_getchar(void) {  // NOLINT
  return -1;
}

void initialise_monitor_handles() {  // NOLINT
}

int _getpid() {  // NOLINT
  return 1;
}

int _kill(int pid, int sig) {  // NOLINT
  (void)pid;
  (void)sig;
  errno = EINVAL;
  return -1;
}

void _exit(int status) {  // NOLINT
  _kill(status, -1);
  while (true) {
    __WFI();
  }
}

int _write(int file, const char* ptr, int len) {  // NOLINT
  (void)file;
  for (int i = 0; i < len; i++) {
    if (__io_putchar(*ptr++) < 0) {
      errno = EIO;
      return -1;
    }
  }
  return len;
}

int _read(int file, char* ptr, int len) {  // NOLINT
  (void)file;
  for (int i = 0; i < len; i++) {
    const int ch = __io_getchar();
    if (ch < 0) {
      errno = EIO;
      return i == 0 ? -1 : i;
    }
    *ptr++ = static_cast<char>(ch);
  }
  return len;
}

int _close(int file) {  // NOLINT
  (void)file;
  return -1;
}

int _fstat(int file, struct stat* st) {  // NOLINT
  (void)file;
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int file) {  // NOLINT
  (void)file;
  return 1;
}

int _lseek(int file, int ptr, int dir) {  // NOLINT
  (void)file;
  (void)ptr;
  (void)dir;
  return 0;
}

int _open(const char* path, int flags, ...) {  // NOLINT
  (void)path;
  (void)flags;
  return -1;
}

int _wait(int* status) {  // NOLINT
  (void)status;
  errno = ECHILD;
  return -1;
}

int _unlink(const char* name) {  // NOLINT
  (void)name;
  errno = ENOENT;
  return -1;
}

int _times(struct tms* buf) {  // NOLINT
  (void)buf;
  return -1;
}

int _stat(const char* file, struct stat* st) {  // NOLINT
  (void)file;
  st->st_mode = S_IFCHR;
  return 0;
}

int _link(const char* oldpath, const char* newpath) {  // NOLINT
  (void)oldpath;
  (void)newpath;
  errno = EMLINK;
  return -1;
}

int _fork() {  // NOLINT
  errno = EAGAIN;
  return -1;
}

int _execve(const char* name, const char** argv, const char** env) {  // NOLINT
  (void)name;
  (void)argv;
  (void)env;
  errno = ENOMEM;
  return -1;
}

// ===== System Memory Calls =====
void* _sbrk(ptrdiff_t incr) {  // NOLINT
  // Defined in linker script
  extern uint8_t _end;
  extern uint8_t _estack;
  extern uint8_t _Min_Stack_Size;  // NOLINT

  static uintptr_t heap_end = reinterpret_cast<uintptr_t>(&_end);  // NOLINT

  const uintptr_t stack_limit =
      reinterpret_cast<uintptr_t>(&_estack) -         // NOLINT
      reinterpret_cast<uintptr_t>(&_Min_Stack_Size);  // NOLINT

  const uintptr_t heap_start = reinterpret_cast<uintptr_t>(&_end);  // NOLINT
  uintptr_t next_heap_end = heap_end;

  if (incr >= 0) {
    const uintptr_t increase = static_cast<uintptr_t>(incr);
    if (heap_end > stack_limit || increase > stack_limit - heap_end) {
      errno = ENOMEM;
      return reinterpret_cast<void*>(static_cast<intptr_t>(-1));  // NOLINT
    }
    next_heap_end += increase;
  } else {
    // Avoid negating PTRDIFF_MIN, which cannot be represented as a
    // positive ptrdiff_t.
    const uintptr_t decrease =
        static_cast<uintptr_t>(-(incr + 1)) + static_cast<uintptr_t>(1);
    if (heap_end < heap_start || decrease > heap_end - heap_start) {
      errno = EINVAL;
      return reinterpret_cast<void*>(static_cast<intptr_t>(-1));  // NOLINT
    }
    next_heap_end -= decrease;
  }

  void* const previous_heap_end = reinterpret_cast<void*>(heap_end);  // NOLINT
  heap_end = next_heap_end;
  return previous_heap_end;
}

}  // extern "C"

}  // namespace nbed::f4

#endif
