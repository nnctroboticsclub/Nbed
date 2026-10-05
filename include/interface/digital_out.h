#pragma once

#include "digital_in.h"

namespace nbed::interface {

class DigitalOut {
 public:
  DigitalOut() = default;
  virtual ~DigitalOut() = default;
  DigitalOut(const DigitalOut&) = delete;
  DigitalOut& operator=(const DigitalOut&) = delete;
  DigitalOut(DigitalOut&&) = delete;
  DigitalOut& operator=(DigitalOut&&) = delete;

  virtual bool Initialize() = 0;

  virtual void Write(bool value) = 0;
  [[nodiscard]] virtual bool Read() const = 0;

  [[nodiscard]] explicit operator bool() const {
    return Read();
  }

  virtual DigitalOut& operator=(bool value) {
    Write(value);
    return *this;
  }

  // DigitalIn の現在の入力値をそのまま出力へ書き込めるようにする。
  DigitalOut& operator=(const DigitalIn& input) {
    Write(input.Read());
    return *this;
  }
};

}  // namespace nbed::interface
