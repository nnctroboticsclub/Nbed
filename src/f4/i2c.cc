#if defined(STM32F446xx)

#include "f4/i2c.h"

#include "stm32f4xx_hal.h"  // IWYU pragma: keep
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_gpio_ex.h"
#include "stm32f4xx_hal_rcc.h"

#include "f4/pin_mapper.h"

namespace nbed::f4 {
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
    __HAL_RCC_I2C1_CLK_ENABLE();  // NOLINT
  } else if (peripheral == I2cPeripheral::kI2c2) {
    __HAL_RCC_I2C2_CLK_ENABLE();  // NOLINT
  } else {
    __HAL_RCC_I2C3_CLK_ENABLE();  // NOLINT
  }
}
}  // namespace

I2c::I2c(I2cPeripheral peripheral, interface::PinName scl,
         interface::PinName sda, uint32_t frequency_hz)
    : peripheral_(peripheral),
      scl_(scl),
      sda_(sda),
      frequency_hz_(frequency_hz) {
}

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
  instance = this;
  EnableI2cClock(peripheral_);
  if (peripheral_ == I2cPeripheral::kI2c1) {         // NOLINT
    handle_.Instance = I2C1;                         // NOLINT
  } else if (peripheral_ == I2cPeripheral::kI2c2) {  // NOLINT
    handle_.Instance = I2C2;                         // NOLINT
  } else {
    handle_.Instance = I2C3;  // NOLINT
  }
  handle_.Init.ClockSpeed = frequency_hz_;
  handle_.Init.DutyCycle = I2C_DUTYCYCLE_2;
  handle_.Init.OwnAddress1 = 0;
  handle_.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  handle_.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  handle_.Init.OwnAddress2 = 0;
  handle_.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  handle_.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&handle_) != HAL_OK) {
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
    return (scl_ == PinName::kPB6 && sda_ == PinName::kPB7) ||
           (scl_ == PinName::kPB8 && sda_ == PinName::kPB9);
  }
  if (peripheral_ == I2cPeripheral::kI2c2) {
    return scl_ == PinName::kPB10 &&
           (sda_ == PinName::kPB3 || sda_ == PinName::kPC12);
  }
  return (scl_ == PinName::kPA8 && sda_ == PinName::kPB4) ||
         (scl_ == PinName::kPA8 && sda_ == PinName::kPC9);
}

}  // namespace nbed::f4

#endif
