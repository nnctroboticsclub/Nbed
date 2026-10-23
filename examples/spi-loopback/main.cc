#include <algorithm>
#include <array>
#include <cstdio>

#include "nbed.h"

// Disconnect external SPI devices, then connect MOSI to MISO:
// F446RE: PA7 (D11) -> PA6 (D12); F303K8: PB5 (D11) -> PB4 (D12).
// F767ZI: PB5 (Zio D22 / Morpho) -> PA6 (D12). PB5 avoids the Ethernet
// PHY connection on PA7 (default D11). See docs/f767zi.md for solder bridges.
// F446RE/F303K8: SCK shares the LED pin; do not initialize a separate LED output.
int main() {
  System::Initialize();
#if defined(STM32F303x8)
  Spi spi(kSpi1, kPB3, kPB4, kPB5, 1'000'000);
#elif defined(STM32F767xx)
  Spi spi(kSpi1, kPA5, kPA6, kPB5, 1'000'000);
#else
  Spi spi(kSpi1, kPA5, kPA6, kPA7, 1'000'000);
#endif
  DigitalOut cs(kD10, true);
  if (!cs.Initialize() || !spi.Initialize()) {
    System::ErrorHandler();
  }
  const std::array<uint8_t, 5> tx{0x00, 0x01, 0x55, 0xaa, 0xff};
  std::array<uint8_t, tx.size()> rx{};
  while (true) {
    cs = false;
    const bool transferred = spi.Transfer(tx, rx, 100);
    const bool sent = spi.Send(tx, 100);
    cs = true;
    const bool matches = transferred && sent && rx == tx;

    cs = false;
    const bool received = spi.Receive(rx, 100);
    cs = true;
    const bool dummy_matches =
        received && std::all_of(rx.begin(), rx.end(),
                                [](uint8_t byte) { return byte == 0xff; });
    printf("SPI loopback: %s (%lu Hz)\n",
           matches && dummy_matches ? "PASS" : "FAIL",
           static_cast<unsigned long>(spi.GetFrequencyHz()));
    SleepFor(1s);
  }
}
