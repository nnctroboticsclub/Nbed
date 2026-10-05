#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>

#include "stm32f4xx_hal.h"  // IWYU pragma: export
#include "stm32f4xx_hal_i2c.h"

#include "interface/i2c.h"
#include "interface/pin_names.h"

namespace nbed::f4 {

enum class I2cPeripheral : uint8_t { kI2c1, kI2c2, kI2c3 };

class I2c final : public interface::I2C {
 public:
  I2c(I2cPeripheral peripheral, interface::PinName scl, interface::PinName sda,
      uint32_t frequency_hz = 100'000);
  ~I2c() override;

  I2c(const I2c&) = delete;
  I2c& operator=(const I2c&) = delete;
  I2c(I2c&&) = delete;
  I2c& operator=(I2c&&) = delete;

  bool Initialize() override;

  bool Send(const interface::I2CMessage& message) override;
  bool Receive(interface::I2CMessage& message) override;

 private:
  [[nodiscard]] bool IsValidPinPair() const;
  I2cPeripheral peripheral_;
  interface::PinName scl_;
  interface::PinName sda_;
  uint32_t frequency_hz_;
  I2C_HandleTypeDef handle_{};
  bool initialized_{false};
};

}  // namespace nbed::f4
