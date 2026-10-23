#pragma once

#include <cstdint>

namespace nkrc::nbed::stm32::f3 {

// Coordinates PWM use of TIM1/2/3/15/16/17 and encoder use of TIM1/2/3.
// PWM channels can coexist on one timer, but an encoder configures the complete
// timer and must therefore be exclusive. Timer IDs follow PwmTimer's order.
class TimerReservation final {
 public:
  TimerReservation() = delete;

  [[nodiscard]] static bool ClaimPwm(uint8_t timer, uint8_t channel);
  static void ReleasePwm(uint8_t timer, uint8_t channel);

  [[nodiscard]] static bool ClaimEncoder(uint8_t timer);
  static void ReleaseEncoder(uint8_t timer);
};

}  // namespace nkrc::nbed::stm32::f3
