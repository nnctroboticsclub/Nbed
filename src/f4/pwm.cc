#if defined(STM32F446xx)

#include "f4/pwm.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_gpio_ex.h"
#include "stm32f4xx_hal_rcc.h"

#include "f4/pin_mapper.h"
#include "f4/timer_reservation.h"

namespace nbed::f4 {

struct Pwm::TimerState {
  TIM_HandleTypeDef handle{};
  uint32_t timer_clock_hz{0};
  uint32_t frequency_hz{0};
  uint8_t active_channels{0};
  uint8_t active_channel_mask{0};
  bool initialized{false};
};

namespace {

struct Timing {
  uint32_t prescaler;  // PSCレジスタ値（分周比 - 1）
  uint32_t period;     // ARRレジスタ値
  uint32_t actual_hz;
};

Pwm::TimerState& StateOf(PwmTimer timer) {
  static Pwm::TimerState tim1{};
  static Pwm::TimerState tim2{};
  static Pwm::TimerState tim3{};
  switch (timer) {
    case PwmTimer::kTim1:
      return tim1;
    case PwmTimer::kTim2:
      return tim2;
    default:
      return tim3;
  }
}

TIM_TypeDef* InstanceOf(PwmTimer timer) {
  switch (timer) {
    case PwmTimer::kTim1:
      return TIM1;  // NOLINT
    case PwmTimer::kTim2:
      return TIM2;  // NOLINT
    default:
      return TIM3;  // NOLINT
  }
}

constexpr uint8_t ChannelBit(PwmChannel channel) {
  switch (channel) {
    case PwmChannel::kCh1:
      return 1U << 0U;
    case PwmChannel::kCh2:
      return 1U << 1U;
    case PwmChannel::kCh3:
      return 1U << 2U;
    case PwmChannel::kCh4:
      return 1U << 3U;
  }
  return 0;
}

// TIM2のみ32bit、TIM1/TIM3は16bit。
constexpr uint32_t MaxPeriod(PwmTimer timer) {
  return timer == PwmTimer::kTim2 ? 0xFFFFFFFFU : 0xFFFFU;
}

void EnableTimerClock(PwmTimer timer) {
  switch (timer) {
    case PwmTimer::kTim1:
      __HAL_RCC_TIM1_CLK_ENABLE();  // NOLINT
      break;
    case PwmTimer::kTim2:
      __HAL_RCC_TIM2_CLK_ENABLE();  // NOLINT
      break;
    case PwmTimer::kTim3:
      __HAL_RCC_TIM3_CLK_ENABLE();  // NOLINT
      break;
  }
}

// 目標周波数からPSC/ARRを計算する。
std::optional<Timing> ComputeTiming(uint32_t clock_hz, uint32_t frequency_hz,
                                    uint32_t max_period) {
  if (frequency_hz == 0) {
    return std::nullopt;
  }
  const uint64_t ticks_per_period = clock_hz / frequency_hz;
  if (ticks_per_period == 0) {
    return std::nullopt;
  }
  const uint64_t counter_range = static_cast<uint64_t>(max_period) + 1ULL;
  const uint64_t divider =
      (ticks_per_period + counter_range - 1ULL) / counter_range;  // ceil
  if (divider == 0 || divider > 0x10000ULL) {
    return std::nullopt;
  }
  const uint64_t counter_ticks = ticks_per_period / divider;
  if (counter_ticks == 0 || counter_ticks > counter_range) {
    return std::nullopt;
  }
  return Timing{
      .prescaler = static_cast<uint32_t>(divider - 1ULL),
      .period = static_cast<uint32_t>(counter_ticks - 1ULL),
      .actual_hz = static_cast<uint32_t>(clock_hz / (divider * counter_ticks)),
  };
}

void SetCompareTicks(Pwm::TimerState& state, PwmTimer timer, uint32_t channel,
                     uint64_t ticks) {
  const uint64_t upper = std::min<uint64_t>(
      static_cast<uint64_t>(state.handle.Init.Period) + 1ULL, MaxPeriod(timer));
  __HAL_TIM_SET_COMPARE(&state.handle, channel,  // NOLINT
                        static_cast<uint32_t>(std::min(ticks, upper)));
}

void ReleaseTimerIfUnused(Pwm::TimerState& state) {
  if (state.active_channels == 0 && state.initialized) {
    (void)HAL_TIM_PWM_DeInit(&state.handle);
    state.initialized = false;
    state.frequency_hz = 0;
    state.timer_clock_hz = 0;
    state.active_channel_mask = 0;
  }
}

}  // namespace

Pwm::Pwm(PwmTimer timer, PwmChannel channel, interface::PinName pin,
         uint32_t frequency_hz)
    : timer_(timer), channel_(channel), pin_(pin), frequency_hz_(frequency_hz) {
}

Pwm::~Pwm() {
  Stop();
}

bool Pwm::Initialize() {  // NOLINT
  if (initialized_) {
    return true;
  }
  if (!IsValidPin() || frequency_hz_ == 0) {
    return false;
  }
  if (!TimerReservation::ClaimPwm(static_cast<uint8_t>(timer_),
                                  static_cast<uint8_t>(channel_))) {
    return false;
  }

  auto& state = State();
  const uint8_t channel_bit = ChannelBit(channel_);
  // A timer channel has one CCR register. Two Pwm instances cannot own it
  // independently: stopping either one would otherwise disable both outputs.
  if ((state.active_channel_mask & channel_bit) != 0U) {
    TimerReservation::ReleasePwm(static_cast<uint8_t>(timer_),
                                 static_cast<uint8_t>(channel_));
    return false;
  }
  const bool first_channel = !state.initialized;
  const auto timing =
      ComputeTiming(first_channel ? TimerClockHz() : state.timer_clock_hz,
                    frequency_hz_, MaxPeriod(timer_));
  if (!timing) {
    TimerReservation::ReleasePwm(static_cast<uint8_t>(timer_),
                                 static_cast<uint8_t>(channel_));
    return false;
  }

  if (first_channel) {
    EnableTimerClock(timer_);
    state.handle.Instance = InstanceOf(timer_);
    state.handle.Init.Prescaler = timing->prescaler;
    state.handle.Init.Period = timing->period;
    state.handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    state.handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    state.handle.Init.RepetitionCounter = 0;
    state.handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&state.handle) != HAL_OK) {
      TimerReservation::ReleasePwm(static_cast<uint8_t>(timer_),
                                   static_cast<uint8_t>(channel_));
      return false;
    }
    state.timer_clock_hz = TimerClockHz();
    state.frequency_hz = timing->actual_hz;
    state.initialized = true;
  } else if (state.handle.Init.Prescaler != timing->prescaler ||
             state.handle.Init.Period != timing->period) {
    TimerReservation::ReleasePwm(static_cast<uint8_t>(timer_),
                                 static_cast<uint8_t>(channel_));
    return false;
  }

  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Alternate = AlternateFunction();
  if (!PinMapper::ConfigureGpio(pin_, gpio)) {
    ReleaseTimerIfUnused(state);
    TimerReservation::ReleasePwm(static_cast<uint8_t>(timer_),
                                 static_cast<uint8_t>(channel_));
    return false;
  }

  TIM_OC_InitTypeDef channel_config{};
  channel_config.OCMode = TIM_OCMODE_PWM1;
  channel_config.Pulse = 0;
  channel_config.OCPolarity = TIM_OCPOLARITY_HIGH;
  channel_config.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&state.handle, &channel_config, HalChannel()) !=
          HAL_OK ||
      HAL_TIM_PWM_Start(&state.handle, HalChannel()) != HAL_OK) {
    ReleaseTimerIfUnused(state);
    TimerReservation::ReleasePwm(static_cast<uint8_t>(timer_),
                                 static_cast<uint8_t>(channel_));
    return false;
  }
  initialized_ = true;
  ++state.active_channels;
  state.active_channel_mask |= channel_bit;
  return true;
}

bool Pwm::SetPulseWidthUs(uint32_t pulse_width_us) {
  if (!initialized_) {
    return false;
  }
  auto& state = State();
  const uint64_t ticks =
      static_cast<uint64_t>(pulse_width_us) * state.timer_clock_hz /
      (static_cast<uint64_t>(state.handle.Init.Prescaler) + 1ULL) /
      1'000'000ULL;
  SetCompareTicks(state, timer_, HalChannel(), ticks);
  return true;
}

bool Pwm::SetFrequencyHz(uint32_t frequency_hz) {
  if (frequency_hz == 0) {
    return false;
  }
  if (!initialized_) {
    frequency_hz_ = frequency_hz;
    return true;
  }
  auto& state = State();
  if (state.active_channels > 1) {
    return false;  // 他のチャンネルと周期を共有しているので変更不可
  }
  const auto timing =
      ComputeTiming(state.timer_clock_hz, frequency_hz, MaxPeriod(timer_));
  if (!timing) {
    return false;
  }

  // 周期が変わってもデューティ比が保たれるようCCRをスケーリングする。
  const uint64_t old_range =
      static_cast<uint64_t>(state.handle.Init.Period) + 1ULL;
  const uint64_t old_compare =
      __HAL_TIM_GET_COMPARE(&state.handle, HalChannel());

  state.handle.Init.Prescaler = timing->prescaler;
  state.handle.Init.Period = timing->period;
  state.frequency_hz = timing->actual_hz;
  frequency_hz_ = frequency_hz;

  __HAL_TIM_SET_PRESCALER(&state.handle, timing->prescaler);
  __HAL_TIM_SET_AUTORELOAD(&state.handle, timing->period);  // NOLINT
  const uint64_t new_range = static_cast<uint64_t>(timing->period) + 1ULL;
  SetCompareTicks(state, timer_, HalChannel(),
                  old_compare * new_range / old_range);
  // PSC/ARR/CCRはプリロードされているので、更新イベントで即時反映する。
  (void)HAL_TIM_GenerateEvent(&state.handle, TIM_EVENTSOURCE_UPDATE);
  return true;
}

bool Pwm::SetDutyCycle(float duty_cycle) {
  if (!initialized_ || std::isnan(duty_cycle)) {
    return false;
  }
  duty_cycle = std::clamp(duty_cycle, 0.0F, 1.0F);
  auto& state = State();
  const double range = static_cast<double>(state.handle.Init.Period) + 1.0;
  // floatのままだとTIM2(32bit)で精度が足りず、1.0でuint32_tに収まらずUBになる。
  const auto ticks = static_cast<uint64_t>(
      std::lround(static_cast<double>(duty_cycle) * range));
  SetCompareTicks(state, timer_, HalChannel(), ticks);
  return true;
}

void Pwm::SendDataDma(std::span<const uint32_t> data) {
  (void)data;
}

uint32_t Pwm::GetFrequencyHz() const {
  return initialized_ ? State().frequency_hz : frequency_hz_;
}

void Pwm::Stop() {
  if (!initialized_) {
    return;
  }
  auto& state = State();
  (void)HAL_TIM_PWM_Stop(&state.handle, HalChannel());
  if (state.active_channels > 0) {
    --state.active_channels;
  }
  state.active_channel_mask &= static_cast<uint8_t>(~ChannelBit(channel_));
  ReleaseTimerIfUnused(state);
  TimerReservation::ReleasePwm(static_cast<uint8_t>(timer_),
                               static_cast<uint8_t>(channel_));
  initialized_ = false;
}

Pwm::TimerState& Pwm::State() const {
  return StateOf(timer_);
}

uint32_t Pwm::HalChannel() const {
  switch (channel_) {
    case PwmChannel::kCh1:
      return TIM_CHANNEL_1;
    case PwmChannel::kCh2:
      return TIM_CHANNEL_2;
    case PwmChannel::kCh3:
      return TIM_CHANNEL_3;
    default:
      return TIM_CHANNEL_4;
  }
}

bool Pwm::IsValidPin() const {  // NOLINT
  if (!PinMapper::IsValidPin(pin_)) {
    return false;
  }
  using interface::PinName;
  if (timer_ == PwmTimer::kTim1) {
    return (channel_ == PwmChannel::kCh1 && pin_ == PinName::kPA8) ||
           (channel_ == PwmChannel::kCh2 && pin_ == PinName::kPA9) ||
           (channel_ == PwmChannel::kCh3 && pin_ == PinName::kPA10) ||
           (channel_ == PwmChannel::kCh4 && pin_ == PinName::kPA11);
  }
  if (timer_ == PwmTimer::kTim2) {
    return (channel_ == PwmChannel::kCh1 &&
            (pin_ == PinName::kPA0 || pin_ == PinName::kPA5 ||
             pin_ == PinName::kPA15 || pin_ == PinName::kPB8)) ||
           (channel_ == PwmChannel::kCh2 &&
            (pin_ == PinName::kPA1 || pin_ == PinName::kPB3 ||
             pin_ == PinName::kPB9)) ||
           (channel_ == PwmChannel::kCh3 &&
            (pin_ == PinName::kPA2 || pin_ == PinName::kPB10)) ||
           (channel_ == PwmChannel::kCh4 &&
            (pin_ == PinName::kPA3 || pin_ == PinName::kPB2));
  }
  return (channel_ == PwmChannel::kCh1 &&
          (pin_ == PinName::kPA6 || pin_ == PinName::kPB4 ||
           pin_ == PinName::kPC6)) ||
         (channel_ == PwmChannel::kCh2 &&
          (pin_ == PinName::kPA7 || pin_ == PinName::kPB5 ||
           pin_ == PinName::kPC7)) ||
         (channel_ == PwmChannel::kCh3 &&
          (pin_ == PinName::kPB0 || pin_ == PinName::kPC8)) ||
         (channel_ == PwmChannel::kCh4 &&
          (pin_ == PinName::kPB1 || pin_ == PinName::kPC9));
}

uint32_t Pwm::AlternateFunction() const {
  switch (timer_) {
    case PwmTimer::kTim1:
    case PwmTimer::kTim2:
      return GPIO_AF1_TIM1;  // GPIO_AF1_TIM2と同じ値
    default:
      return GPIO_AF2_TIM3;
  }
}

uint32_t Pwm::TimerClockHz() const {
  RCC_ClkInitTypeDef clock_config{};
  uint32_t flash_latency = 0;
  HAL_RCC_GetClockConfig(&clock_config, &flash_latency);
  const bool apb2 = timer_ == PwmTimer::kTim1;
  const uint32_t pclk = apb2 ? HAL_RCC_GetPCLK2Freq() : HAL_RCC_GetPCLK1Freq();
  const uint32_t divider =
      apb2 ? clock_config.APB2CLKDivider : clock_config.APB1CLKDivider;
  // APBプリスケーラが1以外のとき、タイマークロックはPCLKの2倍になる。
  return divider == RCC_HCLK_DIV1 ? pclk : pclk * 2U;
}

}  // namespace nbed::f4

#endif
