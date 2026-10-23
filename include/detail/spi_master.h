#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>

#include "interface/spi.h"

#if defined(STM32F303x8)
#include "stm32f3xx_hal.h"
#elif defined(STM32F446xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32F767xx)
#include "stm32f7xx_hal.h"
#endif

namespace nkrc::nbed::detail {

struct SpiTiming {
  uint32_t prescaler;
  uint32_t actual_hz;
};

// Compare before dividing: truncating PCLK/divisor can exceed a requested limit.
[[nodiscard]] constexpr std::optional<SpiTiming> ComputeSpiTiming(
    uint32_t clock_hz, uint32_t requested_hz, uint32_t maximum_hz) {
  if (clock_hz == 0 || requested_hz == 0 || requested_hz > maximum_hz) {
    return std::nullopt;
  }
  constexpr uint32_t prescalers[]{
      SPI_BAUDRATEPRESCALER_2,   SPI_BAUDRATEPRESCALER_4,
      SPI_BAUDRATEPRESCALER_8,   SPI_BAUDRATEPRESCALER_16,
      SPI_BAUDRATEPRESCALER_32,  SPI_BAUDRATEPRESCALER_64,
      SPI_BAUDRATEPRESCALER_128, SPI_BAUDRATEPRESCALER_256};
  uint32_t divisor = 2;
  for (const uint32_t prescaler : prescalers) {
    if (clock_hz <= uint64_t{requested_hz} * divisor) {
      return SpiTiming{prescaler, clock_hz / divisor};
    }
    divisor *= 2;
  }
  return std::nullopt;
}

inline bool ConfigureSpiMaster(SPI_HandleTypeDef& handle, uint32_t prescaler,
                               interface::SpiMode mode,
                               interface::SpiBitOrder bit_order) {
  if (static_cast<unsigned>(mode) > 3 || static_cast<unsigned>(bit_order) > 1) {
    return false;
  }
  const auto mode_bits = static_cast<unsigned>(mode);
  handle.Init.Mode = SPI_MODE_MASTER;
  handle.Init.Direction = SPI_DIRECTION_2LINES;
  handle.Init.DataSize = SPI_DATASIZE_8BIT;
  handle.Init.CLKPolarity =
      (mode_bits & 2U) != 0 ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
  handle.Init.CLKPhase =
      (mode_bits & 1U) != 0 ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
  handle.Init.NSS = SPI_NSS_SOFT;
  handle.Init.BaudRatePrescaler = prescaler;
  handle.Init.FirstBit = bit_order == interface::SpiBitOrder::kMsbFirst
                             ? SPI_FIRSTBIT_MSB
                             : SPI_FIRSTBIT_LSB;
  handle.Init.TIMode = SPI_TIMODE_DISABLE;
  handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  handle.Init.CRCPolynomial = 7;
  return true;
}

inline bool IsValidSpiSize(std::size_t size) {
  return size != 0 && size <= std::numeric_limits<uint16_t>::max();
}

inline bool SpiSend(SPI_HandleTypeDef& handle, std::span<const uint8_t> data,
                    uint32_t timeout_ms) {
  // HAL's blocking transmit does not modify the source despite its signature.
  return IsValidSpiSize(data.size()) &&
         HAL_SPI_Transmit(&handle, const_cast<uint8_t*>(data.data()),
                          static_cast<uint16_t>(data.size()),
                          timeout_ms) == HAL_OK;
}

inline bool SpiTransfer(SPI_HandleTypeDef& handle, std::span<const uint8_t> tx,
                        std::span<uint8_t> rx, uint32_t timeout_ms) {
  if (!IsValidSpiSize(tx.size()) || tx.size() != rx.size()) {
    return false;
  }
  // Exact in-place exchange is supported by HAL. Reject partial overlap before
  // a receive overwrites bytes that have not yet been transmitted.
  const auto tx_begin = reinterpret_cast<uintptr_t>(tx.data());
  const auto rx_begin = reinterpret_cast<uintptr_t>(rx.data());
  const auto distance =
      tx_begin > rx_begin ? tx_begin - rx_begin : rx_begin - tx_begin;
  if (tx_begin != rx_begin && distance < tx.size()) {
    return false;
  }
  return HAL_SPI_TransmitReceive(&handle, const_cast<uint8_t*>(tx.data()),
                                 rx.data(), static_cast<uint16_t>(tx.size()),
                                 timeout_ms) == HAL_OK;
}

inline bool SpiReceive(SPI_HandleTypeDef& handle, std::span<uint8_t> data,
                       uint32_t timeout_ms) {
  if (!IsValidSpiSize(data.size())) {
    return false;
  }
  std::fill(data.begin(), data.end(), uint8_t{0xff});
  return SpiTransfer(handle, data, data, timeout_ms);
}

}  // namespace nkrc::nbed::detail
