#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>
#include <span>

#include "interface/pin_names.h"
#include "interface/spi.h"
#include "stm32f3xx_hal.h"  // IWYU pragma: export
#include "stm32f3xx_hal_spi.h"

namespace nkrc::nbed::stm32::f3 {

enum class SpiPeripheral : uint8_t { kSpi1 };

class Spi final : public interface::Spi {
 public:
  Spi(SpiPeripheral peripheral, interface::PinName sck, interface::PinName miso,
      interface::PinName mosi, uint32_t frequency_hz = 1'000'000,
      interface::SpiMode mode = interface::SpiMode::kMode0,
      interface::SpiBitOrder bit_order = interface::SpiBitOrder::kMsbFirst);
  ~Spi() override;

  Spi(const Spi&) = delete;
  Spi& operator=(const Spi&) = delete;
  Spi(Spi&&) = delete;
  Spi& operator=(Spi&&) = delete;

  bool Initialize() override;
  [[nodiscard]] uint32_t GetFrequencyHz() const override;
  bool Send(std::span<const uint8_t> data, uint32_t timeout_ms = 1000) override;
  bool Receive(std::span<uint8_t> data, uint32_t timeout_ms = 1000) override;
  bool Transfer(std::span<const uint8_t> tx, std::span<uint8_t> rx,
                uint32_t timeout_ms = 1000) override;

 private:
  [[nodiscard]] bool IsValidPinSet() const;

  SpiPeripheral peripheral_;
  interface::PinName sck_;
  interface::PinName miso_;
  interface::PinName mosi_;
  uint32_t frequency_hz_;
  interface::SpiMode mode_;
  interface::SpiBitOrder bit_order_;
  SPI_HandleTypeDef handle_{};
  uint32_t actual_frequency_hz_{0};
  bool initialized_{false};
};

}  // namespace nkrc::nbed::stm32::f3
