#if defined(STM32F767xx)

#include "f7/analog_in.h"

#include <cmath>

#include "f7/pin_mapper.h"
#include "stm32f7xx_hal_adc.h"
#include "stm32f7xx_hal_gpio.h"
#include "stm32f7xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f7 {
namespace {
struct AdcState {
  ADC_HandleTypeDef handle{};
  bool initialized{false};
  uint32_t last_channel{0xffffffffU};
};

AdcState& StateFor(interface::PinName pin) {
  static AdcState adc1, adc3;
  return (static_cast<uint8_t>(pin) >> 4) == 5 ? adc3 : adc1;
}

bool InitializeAdc(interface::PinName pin) {
  AdcState& state = StateFor(pin);
  if (state.initialized) {
    return true;
  }
  ADC_HandleTypeDef& adc = state.handle;
  if ((static_cast<uint8_t>(pin) >> 4) == 5) {
    __HAL_RCC_ADC3_CLK_ENABLE();
    adc.Instance = ADC3;
  } else {
    __HAL_RCC_ADC1_CLK_ENABLE();
    adc.Instance = ADC1;
  }
  // PCLK2 <= 108 MHz: /4 keeps both ADCs below their 36 MHz limit.
  adc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  adc.Init.Resolution = ADC_RESOLUTION_12B;
  adc.Init.ScanConvMode = DISABLE;
  adc.Init.ContinuousConvMode = DISABLE;
  adc.Init.DiscontinuousConvMode = DISABLE;
  adc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  adc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  adc.Init.NbrOfConversion = 1;
  adc.Init.DMAContinuousRequests = DISABLE;
  adc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&adc) != HAL_OK) {
    return false;
  }
  state.initialized = true;
  return true;
}
}  // namespace

AnalogIn::AnalogIn(interface::PinName pin, float reference_voltage)
    : pin_(pin), reference_voltage_(reference_voltage) {}

bool AnalogIn::Initialize() {
  if (initialized_) {
    return true;
  }
  if (!IsValidPin() || !std::isfinite(reference_voltage_) ||
      reference_voltage_ <= 0.0F || !InitializeAdc(pin_)) {
    return false;
  }
  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_ANALOG;
  gpio.Pull = GPIO_NOPULL;
  if (!PinMapper::ConfigureGpio(pin_, gpio)) {
    return false;
  }
  initialized_ = true;
  return true;
}

float AnalogIn::ReadRatio() {
  if (!initialized_) {
    return 0.0F;
  }
  AdcState& state = StateFor(pin_);
  ADC_HandleTypeDef& adc = state.handle;
  ADC_ChannelConfTypeDef channel{};
  channel.Channel = Channel();
  channel.Rank = 1;
  channel.SamplingTime = ADC_SAMPLETIME_15CYCLES;
  if (state.last_channel != channel.Channel) {
    if (HAL_ADC_ConfigChannel(&adc, &channel) != HAL_OK) {
      return 0.0F;
    }
    state.last_channel = channel.Channel;
  }
  if (HAL_ADC_Start(&adc) != HAL_OK) {
    return 0.0F;
  }
  if (HAL_ADC_PollForConversion(&adc, 100) != HAL_OK) {
    (void)HAL_ADC_Stop(&adc);
    return 0.0F;
  }
  const uint32_t value = HAL_ADC_GetValue(&adc);
  (void)HAL_ADC_Stop(&adc);
  return static_cast<float>(value) / 4095.0F;
}

float AnalogIn::ReadVoltage() { return ReadRatio() * reference_voltage_; }

bool AnalogIn::IsValidPin() const {
  return PinMapper::IsValidPin(pin_) && Channel() != 0xffffffffU;
}

uint32_t AnalogIn::Channel() const {
  using interface::PinName;
  switch (pin_) {
    case PinName::kPA0:
      return ADC_CHANNEL_0;
    case PinName::kPA1:
      return ADC_CHANNEL_1;
    case PinName::kPA2:
      return ADC_CHANNEL_2;
    case PinName::kPA3:
      return ADC_CHANNEL_3;
    case PinName::kPA4:
      return ADC_CHANNEL_4;
    case PinName::kPA5:
      return ADC_CHANNEL_5;
    case PinName::kPA6:
      return ADC_CHANNEL_6;
    case PinName::kPA7:
      return ADC_CHANNEL_7;
    case PinName::kPB0:
      return ADC_CHANNEL_8;
    case PinName::kPB1:
      return ADC_CHANNEL_9;
    case PinName::kPC0:
      return ADC_CHANNEL_10;
    case PinName::kPC1:
      return ADC_CHANNEL_11;
    case PinName::kPC2:
      return ADC_CHANNEL_12;
    case PinName::kPC3:
      return ADC_CHANNEL_13;
    case PinName::kPC4:
      return ADC_CHANNEL_14;
    case PinName::kPC5:
      return ADC_CHANNEL_15;
    case PinName::kPF3:
      return ADC_CHANNEL_9;
    case PinName::kPF5:
      return ADC_CHANNEL_15;
    case PinName::kPF10:
      return ADC_CHANNEL_8;
    default:
      return 0xffffffffU;
  }
}

}  // namespace nkrc::nbed::stm32::f7

#endif
