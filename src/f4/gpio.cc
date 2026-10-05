#if defined(STM32F446xx)

#include "f4/gpio.h"

#include "stm32f4xx_hal.h"  // IWYU pragma: keep
#include "stm32f4xx_hal_gpio.h"

#include "f4/pin_mapper.h"

namespace nbed::f4 {
namespace {
uint32_t ToHalPull(GpioPull pull) {
  if (pull == GpioPull::kUp) {
    return GPIO_PULLUP;
  }
  if (pull == GpioPull::kDown) {
    return GPIO_PULLDOWN;
  }
  return GPIO_NOPULL;
}
}  // namespace

DigitalOut::DigitalOut(interface::PinName pin, bool initial_value)
    : pin_(pin), initial_value_(initial_value) {
}

bool DigitalOut::Initialize() {
  if (initialized_) {
    return true;
  }
  if (!PinMapper::IsValidPin(pin_)) {
    return false;
  }
  GPIO_TypeDef* const port = PinMapper::GetGpioPort(pin_);
  const uint16_t pin = PinMapper::GetGpioPin(pin_);
  PinMapper::EnableGpioClock(port);
  HAL_GPIO_WritePin(port, pin, initial_value_ ? GPIO_PIN_SET : GPIO_PIN_RESET);
  GPIO_InitTypeDef config{};
  config.Mode = GPIO_MODE_OUTPUT_PP;
  config.Pull = GPIO_NOPULL;
  config.Speed = GPIO_SPEED_FREQ_LOW;
  if (!PinMapper::ConfigureGpio(pin_, config)) {
    return false;
  }
  initialized_ = true;
  return true;
}

void DigitalOut::Write(bool value) {
  if (initialized_) {
    HAL_GPIO_WritePin(PinMapper::GetGpioPort(pin_), PinMapper::GetGpioPin(pin_),
                      value ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }
}

bool DigitalOut::Read() const {
  return initialized_ &&
         HAL_GPIO_ReadPin(PinMapper::GetGpioPort(pin_),
                          PinMapper::GetGpioPin(pin_)) == GPIO_PIN_SET;
}

DigitalIn::DigitalIn(interface::PinName pin, GpioPull pull)
    : pin_(pin), pull_(pull) {
}

bool DigitalIn::Initialize() {
  if (initialized_) {
    return true;
  }
  if (!PinMapper::IsValidPin(pin_)) {
    return false;
  }
  GPIO_InitTypeDef config{};
  config.Mode = GPIO_MODE_INPUT;
  config.Pull = ToHalPull(pull_);
  if (!PinMapper::ConfigureGpio(pin_, config)) {
    return false;
  }
  initialized_ = true;
  return true;
}

bool DigitalIn::Read() const {
  return initialized_ &&
         HAL_GPIO_ReadPin(PinMapper::GetGpioPort(pin_),
                          PinMapper::GetGpioPin(pin_)) == GPIO_PIN_SET;
}

}  // namespace nbed::f4

#endif
