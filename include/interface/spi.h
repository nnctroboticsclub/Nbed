#pragma once

#include <cstdint>
#include <span>

namespace nkrc::nbed::interface {

enum class SpiMode : uint8_t { kMode0, kMode1, kMode2, kMode3 };
enum class SpiBitOrder : uint8_t { kMsbFirst, kLsbFirst };

// Blocking, full-duplex, 8-bit master. Control CS separately with DigitalOut.
class Spi {
 public:
  Spi() = default;
  virtual ~Spi() = default;
  Spi(const Spi&) = delete;
  Spi& operator=(const Spi&) = delete;
  Spi(Spi&&) = delete;
  Spi& operator=(Spi&&) = delete;

  virtual bool Initialize() = 0;
  [[nodiscard]] virtual uint32_t GetFrequencyHz() const = 0;

  // Each call accepts 1..65535 bytes. Transfer buffers must have equal sizes;
  // use separate buffers or exactly the same buffer (partial overlap is invalid).
  // Timeout is in HAL milliseconds. A failed call may have transferred a prefix.
  virtual bool Send(std::span<const uint8_t> data,
                    uint32_t timeout_ms = 1000) = 0;
  // Sends 0xff for every received byte. On failure the buffer is unspecified.
  virtual bool Receive(std::span<uint8_t> data, uint32_t timeout_ms = 1000) = 0;
  virtual bool Transfer(std::span<const uint8_t> tx, std::span<uint8_t> rx,
                        uint32_t timeout_ms = 1000) = 0;
};

}  // namespace nkrc::nbed::interface
