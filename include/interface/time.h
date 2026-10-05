#pragma once

#include <cstdint>

namespace nbed::interface {

class Timer {
 public:
  Timer() = default;
  virtual ~Timer() = default;
  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;
  Timer(Timer&&) = delete;
  Timer& operator=(Timer&&) = delete;

  virtual void Start() = 0;
  virtual void Stop() = 0;
  virtual void Reset() = 0;

  [[nodiscard]] virtual bool IsRunning() const = 0;
  [[nodiscard]] virtual uint64_t ElapsedMicroseconds() const = 0;
};

}  // namespace nbed::interface
