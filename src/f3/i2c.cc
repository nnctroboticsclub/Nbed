#if defined(STM32F303x8)

#include "f3/i2c.h"

#include "detail/i2c_timing.h"
#include "f3/pin_mapper.h"
#include "stm32f3xx_hal.h"  // IWYU pragma: keep
#include "stm32f3xx_hal_gpio.h"
#include "stm32f3xx_hal_gpio_ex.h"
#include "stm32f3xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f3 {
namespace {
I2c*& InstanceFor(I2cPeripheral peripheral) {
  (void)peripheral;
  static I2c* instance = nullptr;
  return instance;
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
  const auto timing = detail::I2cTimingHsi8Mhz(frequency_hz_);
  if (!timing) {
    return false;
  }
  instance = this;
  // Use the fixed 8 MHz HSI kernel clock independently of the APB clocks.
  __HAL_RCC_I2C1_CONFIG(RCC_I2C1CLKSOURCE_HSI);
  __HAL_RCC_I2C1_CLK_ENABLE();
  handle_.Instance = I2C1;
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
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF4_I2C1;
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
  return peripheral_ == I2cPeripheral::kI2c1 &&
         PinMapper::IsValidPinPair(scl_, sda_) &&
         (scl_ == PinName::kPB6 || scl_ == PinName::kPA15) &&
         (sda_ == PinName::kPB7 || sda_ == PinName::kPA14);
}

}  // namespace nkrc::nbed::stm32::f3

#endif
