#pragma once

#include <cstdint>

namespace nbed::interface {

struct I2CMessage {
  uint16_t address;
  uint8_t size;
  uint8_t* data;
};

class I2C {
 public:
  I2C() = default;
  virtual ~I2C() = default;
  I2C(const I2C&) = delete;
  I2C& operator=(const I2C&) = delete;
  I2C(I2C&&) = delete;
  I2C& operator=(I2C&&) = delete;

  virtual bool Initialize() = 0;

  virtual bool Send(const I2CMessage& message) = 0;
  virtual bool Receive(I2CMessage& message) = 0;
};

}  // namespace nbed::interface
