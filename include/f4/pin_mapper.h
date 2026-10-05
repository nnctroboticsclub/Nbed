#pragma once

#include "stm32f4xx_hal.h"  // IWYU pragma: export

#include "interface/pin_names.h"

namespace nbed::f4 {

class PinMapper {
 public:
  [[nodiscard]] static bool IsValidPin(interface::PinName pin_name);

  [[nodiscard]] static bool IsValidPinPair(interface::PinName first,
                                           interface::PinName second);

  [[nodiscard]] static GPIO_TypeDef* GetGpioPort(interface::PinName pin_name);

  [[nodiscard]] static uint16_t GetGpioPin(interface::PinName pin_name);

  // The GPIO clock is shared by every peripheral on a port. It is therefore
  // only enabled here and is never disabled by an individual object.
  static void EnableGpioClock(GPIO_TypeDef* port);

  // Configures exactly one physical pin and enables its port clock. Keeping
  // this operation here avoids each peripheral having to duplicate the port
  // lookup and clock-enable logic, and also supports pins on different ports.
  [[nodiscard]] static bool ConfigureGpio(interface::PinName pin_name,
                                          const GPIO_InitTypeDef& config);
};

}  // namespace nbed::f4
