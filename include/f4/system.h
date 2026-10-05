#pragma once
// IWYU pragma: private, include "nbed.h"

#include "f4/clock.h"
#include "interface/error.h"

namespace nbed::f4 {

class Error final : public interface::Error {
 public:
  void ErrorHandler() override;
};

class System {
 public:
  static void Initialize();
  static void Initialize(const Clock::Configuration& configuration);

  static void ErrorHandler();
};

}  // namespace nbed::f4
