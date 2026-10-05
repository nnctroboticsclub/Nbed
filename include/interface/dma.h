#pragma once

namespace nbed::interface {

class Dma {
 public:
  Dma() = default;
  virtual ~Dma() = default;
  Dma(const Dma&) = delete;
  Dma& operator=(const Dma&) = delete;
  Dma(Dma&&) = delete;
  Dma& operator=(Dma&&) = delete;

  virtual void Initialize() = 0;
};

}  // namespace nbed::interface
