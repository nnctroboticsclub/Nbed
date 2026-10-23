#if defined(STM32F767xx)

#include "f7/pin_mapper.h"

#include <cstdint>

namespace nkrc::nbed::stm32::f7 {

bool PinMapper::IsValidPin(interface::PinName pin_name) {
  const auto value = static_cast<uint8_t>(pin_name);
  const auto port_index = value >> 4;
  const auto pin_index = value & 0x0F;

  switch (port_index) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      return true;
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
      return GPIOA;
    case 1:
      return GPIOB;
    case 2:
      return GPIOC;
    case 3:
      return GPIOD;
    case 4:
      return GPIOE;
    case 5:
      return GPIOF;
    case 6:
      return GPIOG;
    case 7:
      return GPIOH;
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
  if (port == GPIOA) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
  } else if (port == GPIOB) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
  } else if (port == GPIOC) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
  } else if (port == GPIOD) {
    __HAL_RCC_GPIOD_CLK_ENABLE();
  } else if (port == GPIOE) {
    __HAL_RCC_GPIOE_CLK_ENABLE();
  } else if (port == GPIOF) {
    __HAL_RCC_GPIOF_CLK_ENABLE();
  } else if (port == GPIOG) {
    __HAL_RCC_GPIOG_CLK_ENABLE();
  } else if (port == GPIOH) {
    __HAL_RCC_GPIOH_CLK_ENABLE();
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

}  // namespace nkrc::nbed::stm32::f7

#endif
