#pragma once
// IWYU pragma: private, include "nbed.h"

#include "units.h"

namespace nbed::interface {

class Encoder {
 public:
  Encoder() = default;
  virtual ~Encoder() = default;
  Encoder(const Encoder&) = delete;
  Encoder& operator=(const Encoder&) = delete;
  Encoder(Encoder&&) = delete;
  Encoder& operator=(Encoder&&) = delete;

  virtual bool Initialize() = 0;

  [[nodiscard]] virtual int GetCount() = 0;
  [[nodiscard]] virtual Angle GetAngle() = 0;

  virtual void ResetCount() = 0;
};

}  // namespace nbed::interface
