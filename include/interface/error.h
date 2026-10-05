#pragma once

namespace nbed::interface {

class Error {
 public:
  Error() = default;
  virtual ~Error() = default;
  Error(const Error&) = delete;
  Error& operator=(const Error&) = delete;
  Error(Error&&) = delete;
  Error& operator=(Error&&) = delete;

  virtual void ErrorHandler() = 0;
};

}  // namespace nbed::interface
