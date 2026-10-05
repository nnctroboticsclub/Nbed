#if defined(STM32F446xx)

#include "f4/analog_in.h"

#include <cmath>

#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"

#include "f4/pin_mapper.h"

namespace nbed::f4 {
namespace {
ADC_HandleTypeDef& Adc1() {
  static ADC_HandleTypeDef handle{};
  return handle;
}

bool& Adc1Initialized() {
  static bool initialized = false;
  return initialized;
}

bool InitializeAdc() {
  if (Adc1Initialized()) {
    return true;
  }
  __HAL_RCC_ADC1_CLK_ENABLE();  // NOLINT
  ADC_HandleTypeDef& adc1 = Adc1();
  adc1.Instance = ADC1;  // NOLINT
  // PCLK2 is 90 MHz in the default configuration. /2 would clock the ADC at
  // 45 MHz, above the STM32F446's 36 MHz maximum rating.
  adc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  adc1.Init.Resolution = ADC_RESOLUTION_12B;
  adc1.Init.ScanConvMode = DISABLE;
  adc1.Init.ContinuousConvMode = DISABLE;
  adc1.Init.DiscontinuousConvMode = DISABLE;
  adc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  adc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  adc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  adc1.Init.NbrOfConversion = 1;
  adc1.Init.DMAContinuousRequests = DISABLE;
  adc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&adc1) != HAL_OK) {
    return false;
  }
  Adc1Initialized() = true;
  return true;
}
}  // namespace

AnalogIn::AnalogIn(interface::PinName pin, float reference_voltage)
    : pin_(pin), reference_voltage_(reference_voltage) {
}

bool AnalogIn::Initialize() {
  if (initialized_) {
    return true;
  }
  if (!IsValidPin() || !std::isfinite(reference_voltage_) ||
      reference_voltage_ <= 0.0F || !InitializeAdc()) {
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
  ADC_HandleTypeDef& adc1 = Adc1();
  ADC_ChannelConfTypeDef channel{};
  channel.Channel = Channel();
  channel.Rank = 1;
  channel.SamplingTime = ADC_SAMPLETIME_15CYCLES;
  if (HAL_ADC_ConfigChannel(&adc1, &channel) != HAL_OK ||
      HAL_ADC_Start(&adc1) != HAL_OK) {
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

float AnalogIn::ReadVoltage() {
  return ReadRatio() * reference_voltage_;
}

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
    default:
      return 0xffffffffU;
  }
}

}  // namespace nbed::f4

#endif
