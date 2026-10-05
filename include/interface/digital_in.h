#pragma once

namespace nbed::interface {

class DigitalIn {
 public:
  DigitalIn() = default;
  virtual ~DigitalIn() = default;
  DigitalIn(const DigitalIn&) = delete;
  DigitalIn& operator=(const DigitalIn&) = delete;
  DigitalIn(DigitalIn&&) = delete;
  DigitalIn& operator=(DigitalIn&&) = delete;

  virtual bool Initialize() = 0;

  [[nodiscard]] virtual bool Read() const = 0;

  [[nodiscard]] explicit operator bool() const {
    return Read();
  }

  [[nodiscard]] bool operator==(bool value) const {
    return Read() == value;
  }

  [[nodiscard]] bool operator!=(bool value) const {
    return Read() != value;
  }

  friend bool operator==(bool value, const DigitalIn& input) {
    return input == value;
  }

  friend bool operator!=(bool value, const DigitalIn& input) {
    return input != value;
  }
};

}  // namespace nbed::interface
