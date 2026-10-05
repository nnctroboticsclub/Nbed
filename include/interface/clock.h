#pragma once

#include <chrono>
#include <cstdint>

namespace nbed::interface {

class Clock {
 public:
  Clock() = default;
  virtual ~Clock() = default;
  Clock(const Clock&) = delete;
  Clock& operator=(const Clock&) = delete;
  Clock(Clock&&) = delete;
  Clock& operator=(Clock&&) = delete;

  virtual void Initialize() = 0;

  virtual void Sleep(uint32_t milliseconds) = 0;
  virtual void Sleep(std::chrono::milliseconds milliseconds) = 0;
};

}  // namespace nbed::interface
