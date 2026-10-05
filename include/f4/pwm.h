#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>
#include <span>

#include "stm32f4xx_hal.h"  // IWYU pragma: export

#include "interface/pin_names.h"
#include "interface/pwm.h"

namespace nbed::f4 {

enum class PwmTimer : uint8_t { kTim1, kTim2, kTim3 };
enum class PwmChannel : uint8_t { kCh1, kCh2, kCh3, kCh4 };

// A PWM output. Channels on the same timer necessarily share a frequency.
class Pwm final : public interface::Pwm {
 public:
  // Internal shared state; exposed only because the F4 translation unit owns
  // one state object per hardware timer.
  struct TimerState;

  Pwm(PwmTimer timer, PwmChannel channel, interface::PinName pin,
      uint32_t frequency_hz = 50);
  ~Pwm() override;
  Pwm(const Pwm&) = delete;
  Pwm& operator=(const Pwm&) = delete;
  Pwm(Pwm&&) = delete;
  Pwm& operator=(Pwm&&) = delete;

  bool Initialize() override;

  bool SetPulseWidthUs(uint32_t pulse_width_us) override;
  bool SetFrequencyHz(uint32_t frequency_hz) override;
  bool SetDutyCycle(float duty_cycle) override;

  [[nodiscard]] uint32_t GetFrequencyHz() const override;

  void SendDataDma(std::span<const uint32_t> data) override;

  void Stop() override;

 private:
  [[nodiscard]] TimerState& State() const;
  [[nodiscard]] uint32_t HalChannel() const;
  [[nodiscard]] bool IsValidPin() const;
  [[nodiscard]] uint32_t AlternateFunction() const;
  [[nodiscard]] uint32_t TimerClockHz() const;

  PwmTimer timer_;
  PwmChannel channel_;
  interface::PinName pin_;
  uint32_t frequency_hz_;
  bool initialized_{false};
};

}  // namespace nbed::f4
