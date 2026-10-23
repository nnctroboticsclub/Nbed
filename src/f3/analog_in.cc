#if defined(STM32F303x8)

#include "f3/analog_in.h"

#include <cmath>

#include "f3/pin_mapper.h"
#include "stm32f3xx_hal_adc.h"
#include "stm32f3xx_hal_gpio.h"
#include "stm32f3xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f3 {
namespace {
struct AdcState {
  ADC_HandleTypeDef handle{};
  bool initialized{false};
  uint32_t channel{0xffffffffU};
};

AdcState& StateFor(interface::PinName pin) {
  static AdcState adc1{};
  static AdcState adc2{};
  return pin >= interface::PinName::kPA4 && pin <= interface::PinName::kPA7
             ? adc2
             : adc1;
}

bool InitializeAdc(interface::PinName pin) {
  auto& state = StateFor(pin);
  if (state.initialized) {
    return true;
  }
  __HAL_RCC_ADC12_CLK_ENABLE();
  auto& adc = state.handle;
  adc.Instance =
      pin >= interface::PinName::kPA4 && pin <= interface::PinName::kPA7 ? ADC2
                                                                         : ADC1;
  adc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  adc.Init.Resolution = ADC_RESOLUTION_12B;
  adc.Init.ScanConvMode = ADC_SCAN_DISABLE;
  adc.Init.ContinuousConvMode = DISABLE;
  adc.Init.DiscontinuousConvMode = DISABLE;
  adc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  adc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  adc.Init.NbrOfConversion = 1;
  adc.Init.DMAContinuousRequests = DISABLE;
  adc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  adc.Init.LowPowerAutoWait = DISABLE;
  adc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&adc) != HAL_OK ||
      HAL_ADCEx_Calibration_Start(&adc, ADC_SINGLE_ENDED) != HAL_OK) {
    (void)HAL_ADC_DeInit(&adc);
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
  auto& state = StateFor(pin_);
  auto& adc1 = state.handle;
  ADC_ChannelConfTypeDef channel{};
  channel.Channel = Channel();
  channel.Rank = ADC_REGULAR_RANK_1;
  channel.SingleDiff = ADC_SINGLE_ENDED;
  channel.OffsetNumber = ADC_OFFSET_NONE;
  channel.SamplingTime = ADC_SAMPLETIME_19CYCLES_5;
  if (state.channel != channel.Channel) {
    if (HAL_ADC_ConfigChannel(&adc1, &channel) != HAL_OK) {
      return 0.0F;
    }
    state.channel = channel.Channel;
  }
  if (HAL_ADC_Start(&adc1) != HAL_OK) {
    return 0.0F;
  }
  if (HAL_ADC_PollForConversion(&adc1, 100) != HAL_OK) {
    (void)HAL_ADC_Stop(&adc1);
    return 0.0F;
  }
  const uint32_t value = HAL_ADC_GetValue(&adc1);
  (void)HAL_ADC_Stop(&adc1);
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
      return ADC_CHANNEL_1;
    case PinName::kPA1:
      return ADC_CHANNEL_2;
    case PinName::kPA2:
      return ADC_CHANNEL_3;
    case PinName::kPA3:
      return ADC_CHANNEL_4;
    case PinName::kPA4:
      return ADC_CHANNEL_1;
    case PinName::kPA5:
      return ADC_CHANNEL_2;
    case PinName::kPA6:
      return ADC_CHANNEL_3;
    case PinName::kPA7:
      return ADC_CHANNEL_4;
    case PinName::kPB0:
      return ADC_CHANNEL_11;
    case PinName::kPB1:
      return ADC_CHANNEL_12;
    default:
      return 0xffffffffU;
  }
}

}  // namespace nkrc::nbed::stm32::f3

#endif
