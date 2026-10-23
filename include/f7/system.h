#pragma once
// IWYU pragma: private, include "nbed.h"

#include "f7/clock.h"
#include "interface/error.h"

namespace nkrc::nbed::stm32::f7 {

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

}  // namespace nkrc::nbed::stm32::f7
