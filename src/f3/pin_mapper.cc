#if defined(STM32F303x8)

#include "f3/pin_mapper.h"

#include <cstdint>

namespace nkrc::nbed::stm32::f3 {

bool PinMapper::IsValidPin(interface::PinName pin_name) {
  const auto value = static_cast<uint8_t>(pin_name);
  const auto port = value >> 4;
  const auto pin = value & 0x0f;
  // STM32F303K8T6 (LQFP32), DS9866 figure 4.
  return port == 0 || (port == 1 && (pin <= 1 || (pin >= 3 && pin <= 7))) ||
         (port == 5 && pin <= 1);
}

bool PinMapper::IsValidPinPair(interface::PinName first,
                               interface::PinName second) {
  return IsValidPin(first) && IsValidPin(second) && first != second;
}

GPIO_TypeDef* PinMapper::GetGpioPort(interface::PinName pin_name) {
  if (!IsValidPin(pin_name)) {
    return nullptr;
  }
  switch (static_cast<uint8_t>(pin_name) >> 4) {
    case 0:
      return GPIOA;
    case 1:
      return GPIOB;
    default:
      return GPIOF;
  }
}

uint16_t PinMapper::GetGpioPin(interface::PinName pin_name) {
  return IsValidPin(pin_name)
             ? static_cast<uint16_t>(1U
                                     << (static_cast<uint8_t>(pin_name) & 0x0f))
             : 0;
}

void PinMapper::EnableGpioClock(GPIO_TypeDef* port) {
  if (port == GPIOA) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
  } else if (port == GPIOB) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
  } else if (port == GPIOF) {
    __HAL_RCC_GPIOF_CLK_ENABLE();
  }
}

bool PinMapper::ConfigureGpio(interface::PinName pin_name,
                              const GPIO_InitTypeDef& config) {
  GPIO_TypeDef* const port = GetGpioPort(pin_name);
  if (port == nullptr) {
    return false;
  }
  EnableGpioClock(port);
  GPIO_InitTypeDef pin_config = config;
  pin_config.Pin = GetGpioPin(pin_name);
  HAL_GPIO_Init(port, &pin_config);
  return true;
}

}  // namespace nkrc::nbed::stm32::f3

#endif
