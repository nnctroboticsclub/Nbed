#include <array>
#include <cassert>
#include <cstdint>
#include <limits>

#include "f3/pwm.h"
#include "f3/timer_reservation.h"
#include "f3_pwm_hal_stubs.h"

using namespace nkrc::nbed::stm32::f3;
using nkrc::nbed::interface::PinName;

static_assert(static_cast<uint8_t>(PwmTimer::kTim1) == 0);
static_assert(static_cast<uint8_t>(PwmTimer::kTim2) == 1);
static_assert(static_cast<uint8_t>(PwmTimer::kTim3) == 2);

namespace {
struct PinMapping {
  PwmTimer timer;
  PwmChannel channel;
  PinName pin;
  TIM_TypeDef* instance;
  uint32_t alternate;
};

// DS9866 alternate functions for K8 LQFP32. Complementary outputs
// and pins that exist only on larger packages must not pass as normal PWM.
const std::array kMappings{
    PinMapping{PwmTimer::kTim15, PwmChannel::kCh1, PinName::kPA2, TIM15, 9},
    PinMapping{PwmTimer::kTim15, PwmChannel::kCh2, PinName::kPA3, TIM15, 9},
    PinMapping{PwmTimer::kTim16, PwmChannel::kCh1, PinName::kPA6, TIM16, 1},
    PinMapping{PwmTimer::kTim16, PwmChannel::kCh1, PinName::kPA12, TIM16, 1},
    PinMapping{PwmTimer::kTim16, PwmChannel::kCh1, PinName::kPB4, TIM16, 1},
    PinMapping{PwmTimer::kTim17, PwmChannel::kCh1, PinName::kPA7, TIM17, 1},
    PinMapping{PwmTimer::kTim17, PwmChannel::kCh1, PinName::kPB5, TIM17, 10},
};

void CheckPins() {
  unsigned valid = 0;
  for (auto timer : {PwmTimer::kTim15, PwmTimer::kTim16, PwmTimer::kTim17}) {
    for (unsigned channel = 0; channel < 4; ++channel) {
      for (unsigned pin = 0; pin <= 255; ++pin) {
        const PinMapping* expected = nullptr;
        for (const auto& mapping : kMappings) {
          if (mapping.timer == timer &&
              static_cast<unsigned>(mapping.channel) == channel &&
              static_cast<unsigned>(mapping.pin) == pin) {
            expected = &mapping;
          }
        }
        const auto calls = pwm_init_calls;
        Pwm pwm(timer, static_cast<PwmChannel>(channel),
                static_cast<PinName>(pin), 1000);
        assert(pwm.Initialize() == (expected != nullptr));
        if (expected != nullptr) {
          ++valid;
          assert(pwm.Initialize());
          assert(last_pwm_init.Instance == expected->instance);
          assert(expected->instance->clock_enabled);
          assert(gpio_inits.back().second.Alternate == expected->alternate);
          assert(last_pwm_init.Init.Period <= 65535);
          assert(pwm.GetFrequencyHz() == 1000);
        } else {
          assert(pwm_init_calls == calls);
        }
      }
    }
  }
  assert(valid == kMappings.size());
  Pwm invalid(static_cast<PwmTimer>(255), PwmChannel::kCh1, PinName::kPA2);
  assert(!invalid.Initialize());
}

void CheckSharing() {
  Pwm first(PwmTimer::kTim15, PwmChannel::kCh1, PinName::kPA2, 1000);
  assert(first.Initialize() && first.SetDutyCycle(0.25F));
  const auto deinit_calls = pwm_deinit_calls;
  Pwm conflict(PwmTimer::kTim15, PwmChannel::kCh2, PinName::kPA3, 2000);
  assert(!conflict.Initialize());
  assert(pwm_deinit_calls == deinit_calls && tim15.active_channels == 1);
  Pwm second(PwmTimer::kTim15, PwmChannel::kCh2, PinName::kPA3, 1000);
  assert(second.Initialize() && second.SetDutyCycle(0.75F));
  assert(tim15.active_channels == 3);
  assert(tim15.CCR[0] == 16000 && tim15.CCR[1] == 48000);
  assert(!first.SetFrequencyHz(2000));
  first.Stop();
  assert(pwm_deinit_calls == deinit_calls && tim15.active_channels == 2);
  assert(second.SetFrequencyHz(2000));
  assert(tim15.ARR == 31999 && tim15.CCR[1] == 24000);
  second.Stop();
  assert(pwm_deinit_calls == deinit_calls + 1 && tim15.active_channels == 0);
  assert(conflict.Initialize());

  Pwm owner(PwmTimer::kTim16, PwmChannel::kCh1, PinName::kPA6, 1000);
  Pwm duplicate(PwmTimer::kTim16, PwmChannel::kCh1, PinName::kPB4, 1000);
  Pwm independent(PwmTimer::kTim17, PwmChannel::kCh1, PinName::kPB5, 2000);
  assert(owner.Initialize() && !duplicate.Initialize());
  assert(independent.Initialize());
  owner.Stop();
  assert(duplicate.Initialize());
  assert(tim17.active_channels == 1 && independent.GetFrequencyHz() == 2000);
}

void CheckTiming() {
  for (auto timer : {PwmTimer::kTim1, PwmTimer::kTim2, PwmTimer::kTim3,
                     PwmTimer::kTim15, PwmTimer::kTim16, PwmTimer::kTim17}) {
    const auto pin = timer == PwmTimer::kTim1    ? PinName::kPA8
                     : timer == PwmTimer::kTim2  ? PinName::kPA0
                     : timer == PwmTimer::kTim3  ? PinName::kPA6
                     : timer == PwmTimer::kTim15 ? PinName::kPA2
                     : timer == PwmTimer::kTim16 ? PinName::kPA6
                                                 : PinName::kPA7;
    const bool apb2 = timer != PwmTimer::kTim2 && timer != PwmTimer::kTim3;
    for (uint32_t divider : {RCC_HCLK_DIV1, RCC_HCLK_DIV2, RCC_HCLK_DIV4}) {
      pwm_clock_config.APB1CLKDivider = RCC_HCLK_DIV2;
      pwm_clock_config.APB2CLKDivider = divider;
      pclk1_hz = 12'000'000;
      pclk2_hz = 48'000'000 / divider;
      const uint32_t clock =
          apb2 ? pclk2_hz * (divider == 1 ? 1 : 2) : pclk1_hz * 2;
      Pwm pwm(timer, PwmChannel::kCh1, pin, 1000);
      assert(pwm.Initialize());
      assert(uint64_t{last_pwm_init.Init.Prescaler + 1} *
                 (last_pwm_init.Init.Period + 1) ==
             clock / 1000);
      assert(pwm.SetPulseWidthUs(100));
      assert(last_pwm_init.Instance->CCR[0] == clock / 10000);
    }
  }
  pclk1_hz = 32'000'000;
  pclk2_hz = 64'000'000;
  pwm_clock_config = {};
  Pwm pwm(PwmTimer::kTim17, PwmChannel::kCh1, PinName::kPA7, 1);
  assert(!pwm.SetDutyCycle(0.5F) && !pwm.SetPulseWidthUs(50));
  assert(pwm.Initialize());
  assert(tim17.ARR <= 65535 && tim17.PSC <= 65535);
  assert(!pwm.SetDutyCycle(std::numeric_limits<float>::quiet_NaN()));
  assert(pwm.SetFrequencyHz(1000));
  assert(pwm.SetDutyCycle(-1.0F) && tim17.CCR[0] == 0);
  assert(pwm.SetDutyCycle(2.0F) && tim17.CCR[0] == 64000);
  assert(!pwm.SetFrequencyHz(0) && !pwm.SetFrequencyHz(64'000'001));
  assert(pwm.GetFrequencyHz() == 1000);
  assert(pwm_updates > 0);
}

void CheckFailures() {
  for (auto* status :
       {&pwm_init_status, &pwm_config_status, &pwm_start_status}) {
    Pwm pwm(PwmTimer::kTim16, PwmChannel::kCh1, PinName::kPB4, 1000);
    *status = HAL_ERROR;
    assert(!pwm.Initialize());
    *status = HAL_OK;
    assert(pwm.Initialize());
    pwm.Stop();
    pwm.Stop();
    assert(tim16.active_channels == 0);
  }
  for (uint8_t timer = 3; timer < 6; ++timer) {
    assert(!TimerReservation::ClaimEncoder(timer));
    assert(TimerReservation::ClaimPwm(timer, 0));
    assert(!TimerReservation::ClaimPwm(timer, 0));
    assert(!TimerReservation::ClaimPwm(timer, timer == 3 ? 2 : 1));
    TimerReservation::ReleaseEncoder(timer);
    assert(!TimerReservation::ClaimPwm(timer, 0));
    TimerReservation::ReleasePwm(timer, 0);
  }
  assert(!TimerReservation::ClaimPwm(6, 0));
  assert(!TimerReservation::ClaimPwm(255, 255));
  assert(TimerReservation::ClaimEncoder(0));
  Pwm blocked(PwmTimer::kTim1, PwmChannel::kCh1, PinName::kPA8, 1000);
  assert(!blocked.Initialize());
  TimerReservation::ReleaseEncoder(0);
  assert(blocked.Initialize());
}
}  // namespace

int main() {
  CheckPins();
  CheckSharing();
  CheckTiming();
  CheckFailures();
}
