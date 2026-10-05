#if defined(STM32F446xx)

#include "f4/pin_mapper.h"

#include <cstdint>

namespace nbed::f4 {

bool PinMapper::IsValidPin(interface::PinName pin_name) {
  const auto value = static_cast<uint8_t>(pin_name);
  const auto port_index = value >> 4;
  const auto pin_index = value & 0x0F;

  switch (port_index) {
    case 0:
    case 2:
      return true;
    case 1:
      return pin_index <= 10 || pin_index >= 12;
    case 3:
      return pin_index == 2;
    case 7:
      return pin_index <= 1;
    default:
      return false;
  }
}

bool PinMapper::IsValidPinPair(interface::PinName first,
                               interface::PinName second) {
  return IsValidPin(first) && IsValidPin(second) && first != second;
}

GPIO_TypeDef* PinMapper::GetGpioPort(interface::PinName pin_name) {
  if (!IsValidPin(pin_name)) {
    return nullptr;
  }
  auto port_index = static_cast<uint8_t>(pin_name) >> 4;
  switch (port_index) {
    case 0:
      return GPIOA;  // NOLINT
    case 1:
      return GPIOB;  // NOLINT
    case 2:
      return GPIOC;  // NOLINT
    case 3:
      return GPIOD;  // NOLINT
    case 7:
      return GPIOH;  // NOLINT
    default:
      return nullptr;
  }
}

uint16_t PinMapper::GetGpioPin(interface::PinName pin_name) {
  if (!IsValidPin(pin_name)) {
    return 0;
  }
  auto pin_index = static_cast<uint8_t>(pin_name) & 0x0F;
  return 1U << pin_index;
}

void PinMapper::EnableGpioClock(GPIO_TypeDef* port) {
  if (port == GPIOA) {             // NOLINT
    __HAL_RCC_GPIOA_CLK_ENABLE();  // NOLINT
  } else if (port == GPIOB) {      // NOLINT
    __HAL_RCC_GPIOB_CLK_ENABLE();  // NOLINT
  } else if (port == GPIOC) {      // NOLINT
    __HAL_RCC_GPIOC_CLK_ENABLE();  // NOLINT
  } else if (port == GPIOD) {      // NOLINT
    __HAL_RCC_GPIOD_CLK_ENABLE();  // NOLINT
  } else if (port == GPIOH) {      // NOLINT
    __HAL_RCC_GPIOH_CLK_ENABLE();  // NOLINT
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

}  // namespace nbed::f4

#endif
