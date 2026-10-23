#if defined(STM32F303x8)

#include "f3/spi.h"

#include "detail/spi_master.h"
#include "f3/pin_mapper.h"
#include "stm32f3xx_hal_gpio_ex.h"
#include "stm32f3xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f3 {
namespace {
Spi*& Spi1Instance() {
  static Spi* instance = nullptr;
  return instance;
}
}  // namespace

Spi::Spi(SpiPeripheral peripheral, interface::PinName sck,
         interface::PinName miso, interface::PinName mosi,
         uint32_t frequency_hz, interface::SpiMode mode,
         interface::SpiBitOrder bit_order)
    : peripheral_(peripheral),
      sck_(sck),
      miso_(miso),
      mosi_(mosi),
      frequency_hz_(frequency_hz),
      mode_(mode),
      bit_order_(bit_order) {}

Spi::~Spi() {
  if (initialized_) {
    (void)HAL_SPI_DeInit(&handle_);
    Spi1Instance() = nullptr;
  }
}

bool Spi::Initialize() {
  if (initialized_) {
    return true;
  }
  if (!IsValidPinSet() || Spi1Instance() != nullptr) {
    return false;
  }
  // DS9866: master SCK <= 24 MHz at the Nucleo's 3.3 V supply.
  const auto timing = detail::ComputeSpiTiming(HAL_RCC_GetPCLK2Freq(),
                                               frequency_hz_, 24'000'000);
  if (!timing || !detail::ConfigureSpiMaster(handle_, timing->prescaler, mode_,
                                             bit_order_)) {
    return false;
  }
  Spi1Instance() = this;
  __HAL_RCC_SPI1_CLK_ENABLE();
  handle_.Instance = SPI1;
  handle_.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  handle_.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;

  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF5_SPI1;
  if (!PinMapper::ConfigureGpio(sck_, gpio) ||
      !PinMapper::ConfigureGpio(miso_, gpio) ||
      !PinMapper::ConfigureGpio(mosi_, gpio) ||
      HAL_SPI_Init(&handle_) != HAL_OK) {
    (void)HAL_SPI_DeInit(&handle_);
    Spi1Instance() = nullptr;
    return false;
  }
  // Drive the configured idle SCK level before the caller asserts CS.
  __HAL_SPI_ENABLE(&handle_);
  actual_frequency_hz_ = timing->actual_hz;
  initialized_ = true;
  return true;
}

uint32_t Spi::GetFrequencyHz() const { return actual_frequency_hz_; }

bool Spi::Send(std::span<const uint8_t> data, uint32_t timeout_ms) {
  return initialized_ && detail::SpiSend(handle_, data, timeout_ms);
}

bool Spi::Receive(std::span<uint8_t> data, uint32_t timeout_ms) {
  return initialized_ && detail::SpiReceive(handle_, data, timeout_ms);
}

bool Spi::Transfer(std::span<const uint8_t> tx, std::span<uint8_t> rx,
                   uint32_t timeout_ms) {
  return initialized_ && detail::SpiTransfer(handle_, tx, rx, timeout_ms);
}

bool Spi::IsValidPinSet() const {
  using interface::PinName;
  return peripheral_ == SpiPeripheral::kSpi1 &&
         (sck_ == PinName::kPA5 || sck_ == PinName::kPB3) &&
         (miso_ == PinName::kPA6 || miso_ == PinName::kPB4) &&
         (mosi_ == PinName::kPA7 || mosi_ == PinName::kPB5);
}

}  // namespace nkrc::nbed::stm32::f3

#endif
