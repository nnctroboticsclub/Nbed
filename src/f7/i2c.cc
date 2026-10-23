#if defined(STM32F767xx)

#include "f7/i2c.h"

#include "detail/i2c_timing.h"
#include "f7/pin_mapper.h"
#include "stm32f7xx_hal.h"  // IWYU pragma: keep
#include "stm32f7xx_hal_gpio.h"
#include "stm32f7xx_hal_gpio_ex.h"
#include "stm32f7xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f7 {
namespace {
I2c*& I2c1Instance() {
  static I2c* instance = nullptr;
  return instance;
}

I2c*& I2c2Instance() {
  static I2c* instance = nullptr;
  return instance;
}

I2c*& I2c3Instance() {
  static I2c* instance = nullptr;
  return instance;
}

I2c*& InstanceFor(I2cPeripheral peripheral) {
  if (peripheral == I2cPeripheral::kI2c1) {
    return I2c1Instance();
  }
  if (peripheral == I2cPeripheral::kI2c2) {
    return I2c2Instance();
  }
  return I2c3Instance();
}

void EnableI2cClock(I2cPeripheral peripheral) {
  if (peripheral == I2cPeripheral::kI2c1) {
    __HAL_RCC_I2C1_CONFIG(RCC_I2C1CLKSOURCE_HSI);
    __HAL_RCC_I2C1_CLK_ENABLE();
  } else if (peripheral == I2cPeripheral::kI2c2) {
    __HAL_RCC_I2C2_CONFIG(RCC_I2C2CLKSOURCE_HSI);
    __HAL_RCC_I2C2_CLK_ENABLE();
  } else {
    __HAL_RCC_I2C3_CONFIG(RCC_I2C3CLKSOURCE_HSI);
    __HAL_RCC_I2C3_CLK_ENABLE();
  }
}
}  // namespace

I2c::I2c(I2cPeripheral peripheral, interface::PinName scl,
         interface::PinName sda, uint32_t frequency_hz)
    : peripheral_(peripheral),
      scl_(scl),
      sda_(sda),
      frequency_hz_(frequency_hz) {}

I2c::~I2c() {
  if (!initialized_) {
    return;
  }
  (void)HAL_I2C_DeInit(&handle_);
  if (InstanceFor(peripheral_) == this) {
    InstanceFor(peripheral_) = nullptr;
  }
  initialized_ = false;
}

bool I2c::Initialize() {
  if (initialized_) {
    return true;
  }
  I2c*& instance = InstanceFor(peripheral_);
  if (!IsValidPinPair() || frequency_hz_ == 0 || frequency_hz_ > 400'000 ||
      (instance != nullptr && instance != this)) {
    return false;
  }
  const auto timing = detail::I2cTimingHsi16Mhz(frequency_hz_);
  if (!timing) {
    return false;
  }
  instance = this;
  EnableI2cClock(peripheral_);
  if (peripheral_ == I2cPeripheral::kI2c1) {
    handle_.Instance = I2C1;
  } else if (peripheral_ == I2cPeripheral::kI2c2) {
    handle_.Instance = I2C2;
  } else {
    handle_.Instance = I2C3;
  }
  handle_.Init.Timing = *timing;
  handle_.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  handle_.Init.OwnAddress1 = 0;
  handle_.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  handle_.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  handle_.Init.OwnAddress2 = 0;
  handle_.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  handle_.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&handle_) != HAL_OK ||
      HAL_I2CEx_ConfigAnalogFilter(&handle_, I2C_ANALOGFILTER_ENABLE) !=
          HAL_OK ||
      HAL_I2CEx_ConfigDigitalFilter(&handle_, 0) != HAL_OK) {
    (void)HAL_I2C_DeInit(&handle_);
    instance = nullptr;
    return false;
  }
  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_OD;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF4_I2C1;  // I2C1, I2C2, I2C3 all use AF4
  if (!PinMapper::ConfigureGpio(scl_, gpio) ||
      !PinMapper::ConfigureGpio(sda_, gpio)) {
    (void)HAL_I2C_DeInit(&handle_);
    instance = nullptr;
    return false;
  }
  initialized_ = true;
  return true;
}

bool I2c::Send(const interface::I2CMessage& message) {
  return initialized_ && message.address <= 0x7fU && message.data != nullptr &&
         message.size != 0 &&
         HAL_I2C_Master_Transmit(&handle_, message.address << 1, message.data,
                                 message.size, HAL_MAX_DELAY) == HAL_OK;
}

bool I2c::Receive(interface::I2CMessage& message) {
  return initialized_ && message.address <= 0x7fU && message.data != nullptr &&
         message.size != 0 &&
         HAL_I2C_Master_Receive(&handle_, message.address << 1, message.data,
                                message.size, HAL_MAX_DELAY) == HAL_OK;
}

bool I2c::IsValidPinPair() const {
  using interface::PinName;
  if (!PinMapper::IsValidPinPair(scl_, sda_)) {
    return false;
  }
  if (peripheral_ == I2cPeripheral::kI2c1) {
    return (scl_ == PinName::kPB6 || scl_ == PinName::kPB8) &&
           (sda_ == PinName::kPB7 || sda_ == PinName::kPB9);
  }
  if (peripheral_ == I2cPeripheral::kI2c2) {
    return (scl_ == PinName::kPB10 || scl_ == PinName::kPF1) &&
           (sda_ == PinName::kPB11 || sda_ == PinName::kPF0);
  }
  return peripheral_ == I2cPeripheral::kI2c3 && scl_ == PinName::kPA8 &&
         sda_ == PinName::kPC9;
}

}  // namespace nkrc::nbed::stm32::f7

#endif
