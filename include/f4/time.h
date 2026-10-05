#pragma once

#include <cstdint>

#include "interface/time.h"

namespace nbed::f4 {

// Monotonic wall time based on the HAL millisecond tick plus DWT sub-ms time.
class Time {
 public:
  static bool Initialize();

  // HAL glue called after HAL_IncTick() from SysTick_Handler.
  static void OnSysTick();

  [[nodiscard]] static uint32_t Milliseconds();
  [[nodiscard]] static uint64_t Microseconds();
};

// Mbed-style elapsed timer. Time::Initialize must be called after clocks are
// configured, normally immediately after System::InitializeAll().
class Timer final : public interface::Timer {
 public:
  void Start() override;
  void Stop() override;
  void Reset() override;

  [[nodiscard]] bool IsRunning() const override;
  [[nodiscard]] uint64_t ElapsedMicroseconds() const override;

 private:
  uint64_t elapsed_us_{0};
  uint64_t started_at_us_{0};
  bool running_{false};
};

}  // namespace nbed::f4
