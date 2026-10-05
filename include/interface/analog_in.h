#pragma once

namespace nbed::interface {

class AnalogIn {
 public:
  AnalogIn() = default;
  virtual ~AnalogIn() = default;
  AnalogIn(const AnalogIn&) = delete;
  AnalogIn& operator=(const AnalogIn&) = delete;
  AnalogIn(AnalogIn&&) = delete;
  AnalogIn& operator=(AnalogIn&&) = delete;

  virtual bool Initialize() = 0;

  [[nodiscard]] virtual float ReadRatio() = 0;
  [[nodiscard]] virtual float ReadVoltage() = 0;
};

}  // namespace nbed::interface
