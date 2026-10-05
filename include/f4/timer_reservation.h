#pragma once

#include <cstdint>

namespace nbed::f4 {

// Coordinates exclusive use of TIM1, TIM2, and TIM3 by the PWM and encoder
// implementations. PWM channels can coexist on one timer, but an encoder
// configures the complete timer and must therefore be exclusive.
class TimerReservation final {
 public:
  TimerReservation() = delete;

  [[nodiscard]] static bool ClaimPwm(uint8_t timer, uint8_t channel);
  static void ReleasePwm(uint8_t timer, uint8_t channel);

  [[nodiscard]] static bool ClaimEncoder(uint8_t timer);
  static void ReleaseEncoder(uint8_t timer);
};

}  // namespace nbed::f4
