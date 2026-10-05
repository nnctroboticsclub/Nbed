#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>

#include "interface/digital_in.h"
#include "interface/digital_out.h"
#include "interface/pin_names.h"

namespace nbed::f4 {

enum class GpioPull : uint8_t { kNone, kUp, kDown };

class DigitalOut final : public interface::DigitalOut {
 public:
  // DigitalIn の現在の入力値をそのまま出力へ書き込めるようにする。
  using interface::DigitalOut::operator=;

  explicit DigitalOut(interface::PinName pin, bool initial_value = false);

  bool Initialize() override;

  void Write(bool value) override;
  [[nodiscard]] bool Read() const override;

  DigitalOut& operator=(bool value) override {
    Write(value);
    return *this;
  }

 private:
  interface::PinName pin_;
  bool initial_value_;
  bool initialized_{false};
};

class DigitalIn final : public interface::DigitalIn {
 public:
  explicit DigitalIn(interface::PinName pin, GpioPull pull = GpioPull::kNone);

  bool Initialize() override;

  [[nodiscard]] bool Read() const override;

 private:
  interface::PinName pin_;
  GpioPull pull_;
  bool initialized_{false};
};

}  // namespace nbed::f4
