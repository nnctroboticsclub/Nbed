#if defined(STM32F446xx)

#include "f4/encoder.h"

#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_gpio_ex.h"
#include "stm32f4xx_hal_rcc.h"

#include "f4/pin_mapper.h"
#include "f4/timer_reservation.h"

namespace nbed::f4 {
namespace {
void EnableTimerClock(EncoderTimer timer) {
  if (timer == EncoderTimer::kTim1) {
    __HAL_RCC_TIM1_CLK_ENABLE();  // NOLINT
  } else if (timer == EncoderTimer::kTim2) {
    __HAL_RCC_TIM2_CLK_ENABLE();  // NOLINT
  } else {
    __HAL_RCC_TIM3_CLK_ENABLE();  // NOLINT
  }
}
}  // namespace

Encoder::Encoder(EncoderTimer timer, interface::PinName channel_a,
                 interface::PinName channel_b, uint32_t counts_per_revolution)
    : timer_(timer),
      channel_a_(channel_a),
      channel_b_(channel_b),
      counts_per_revolution_(counts_per_revolution) {
}

Encoder::~Encoder() {
  if (!initialized_) {
    return;
  }
  (void)HAL_TIM_Encoder_Stop(&handle_, TIM_CHANNEL_ALL);
  (void)HAL_TIM_Encoder_DeInit(&handle_);
  TimerReservation::ReleaseEncoder(static_cast<uint8_t>(timer_));
  initialized_ = false;
}

bool Encoder::Initialize() {  // NOLINT
  if (initialized_) {
    return true;
  }
  if (!IsValidPinPair() || counts_per_revolution_ == 0) {
    return false;
  }
  if (!TimerReservation::ClaimEncoder(static_cast<uint8_t>(timer_))) {
    return false;
  }
  EnableTimerClock(timer_);
  if (timer_ == EncoderTimer::kTim1) {         // NOLINT
    handle_.Instance = TIM1;                   // NOLINT
  } else if (timer_ == EncoderTimer::kTim2) {  // NOLINT
    handle_.Instance = TIM2;                   // NOLINT
  } else {
    handle_.Instance = TIM3;  // NOLINT
  }
  handle_.Init.Prescaler = 0;
  handle_.Init.CounterMode = TIM_COUNTERMODE_UP;
  handle_.Init.Period = timer_ == EncoderTimer::kTim2 ? 0xffffffffU : 0xffffU;
  handle_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  handle_.Init.RepetitionCounter = 0;
  handle_.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  TIM_Encoder_InitTypeDef config{};
  config.EncoderMode = TIM_ENCODERMODE_TI12;
  config.IC1Polarity = TIM_ICPOLARITY_RISING;
  config.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  config.IC1Prescaler = TIM_ICPSC_DIV1;
  config.IC1Filter = 0;
  config.IC2Polarity = TIM_ICPOLARITY_RISING;
  config.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  config.IC2Prescaler = TIM_ICPSC_DIV1;
  config.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&handle_, &config) != HAL_OK ||
      HAL_TIM_Encoder_Start(&handle_, TIM_CHANNEL_ALL) != HAL_OK) {
    (void)HAL_TIM_Encoder_DeInit(&handle_);
    TimerReservation::ReleaseEncoder(static_cast<uint8_t>(timer_));
    return false;
  }
  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  if (timer_ == EncoderTimer::kTim1) {  // NOLINT
    gpio.Alternate = GPIO_AF1_TIM1;
  } else if (timer_ == EncoderTimer::kTim2) {
    gpio.Alternate = GPIO_AF1_TIM2;
  } else {
    gpio.Alternate = GPIO_AF2_TIM3;
  }
  if (!PinMapper::ConfigureGpio(channel_a_, gpio) ||
      !PinMapper::ConfigureGpio(channel_b_, gpio)) {
    (void)HAL_TIM_Encoder_Stop(&handle_, TIM_CHANNEL_ALL);
    (void)HAL_TIM_Encoder_DeInit(&handle_);
    TimerReservation::ReleaseEncoder(static_cast<uint8_t>(timer_));
    return false;
  }
  initialized_ = true;
  return true;
}

int Encoder::GetCount() {
  if (!initialized_) {
    return 0;
  }
  const uint32_t count = __HAL_TIM_GET_COUNTER(&handle_);
  return timer_ == EncoderTimer::kTim2 ? static_cast<int32_t>(count)
                                       : static_cast<int16_t>(count);
}

Angle Encoder::GetAngle() {
  return Angle::FromDegree(static_cast<float>(GetCount()) * 360.0F /
                           static_cast<float>(counts_per_revolution_));
}

bool Encoder::IsValidPinPair() const {
  using interface::PinName;
  if (!PinMapper::IsValidPinPair(channel_a_, channel_b_)) {
    return false;
  }
  if (timer_ == EncoderTimer::kTim1) {
    return channel_a_ == PinName::kPA8 && channel_b_ == PinName::kPA9;
  }
  if (timer_ == EncoderTimer::kTim2) {
    return (channel_a_ == PinName::kPA0 || channel_a_ == PinName::kPA5 ||
            channel_a_ == PinName::kPA15 || channel_a_ == PinName::kPB8) &&
           (channel_b_ == PinName::kPA1 || channel_b_ == PinName::kPB3 ||
            channel_b_ == PinName::kPB9);
  }
  return (channel_a_ == PinName::kPA6 || channel_a_ == PinName::kPB4 ||
          channel_a_ == PinName::kPC6) &&
         (channel_b_ == PinName::kPA7 || channel_b_ == PinName::kPB5 ||
          channel_b_ == PinName::kPC7);
}

void Encoder::ResetCount() {
  if (initialized_) {
    __HAL_TIM_SET_COUNTER(&handle_, 0);
  }
}

}  // namespace nbed::f4

#endif
