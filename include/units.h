#pragma once

#include <numbers>

namespace nbed {

inline constexpr float kPi = std::numbers::pi_v<float>;

using RadPS = float;
using Rpm = float;

using Degree = float;
using Rad = float;

using Mps = float;

class Velocity {
 public:
  static constexpr Velocity FromRadPS(RadPS rad_ps) {
    Velocity velocity;
    velocity.SetAsRadPS(rad_ps);
    return velocity;
  }

  static constexpr Velocity FromRpm(Rpm rpm) {
    Velocity velocity;
    velocity.SetAsRpm(rpm);
    return velocity;
  }

  constexpr void SetAsRadPS(RadPS rad_ps) {
    rad_ps_ = rad_ps;
  }

  constexpr void SetAsRpm(Rpm rpm) {
    rad_ps_ = rpm * (2.0f * kPi) / 60.0f;
  }

  [[nodiscard]] constexpr RadPS GetAsRadPS() const {
    return rad_ps_;
  }

  [[nodiscard]] constexpr Rpm GetAsRpm() const {
    return rad_ps_ * 60.0f / (2.0f * kPi);
  }

 private:
  RadPS rad_ps_ = 0.0f;  // 角速度 [rad/s]
};

class Current {
 public:
  static constexpr Current FromAmpere(float amp) {
    Current current;
    current.SetAsAmpere(amp);
    return current;
  }

  static constexpr Current FromMilliAmpere(float mA) {
    Current current;
    current.SetAsMilliAmpere(mA);
    return current;
  }

  constexpr void SetAsAmpere(float amp) {
    ampere_ = amp;
  }

  constexpr void SetAsMilliAmpere(float mA) {
    ampere_ = mA / 1000.0f;
  }

  [[nodiscard]] constexpr float GetAsAmpere() const {
    return ampere_;
  }

  [[nodiscard]] constexpr float GetAsMilliAmpere() const {
    return ampere_ * 1000.0f;
  }

 private:
  float ampere_ = 0.0f;  // 電流 [A]
};

class Torque {
 public:
  constexpr void SetAsNm(float nm) {
    nm_ = nm;
  }

  constexpr void SetAsMNm(float mNm) {
    nm_ = mNm / 1000.0f;
  }

  [[nodiscard]] constexpr float GetAsNm() const {
    return nm_;
  }

  [[nodiscard]] constexpr float GetAsMNm() const {
    return nm_ * 1000.0f;
  }

 private:
  float nm_ = 0.0f;  // トルク [N・m]
};

class Angle {
 public:
  static constexpr Angle FromRadian(Rad radian) {
    Angle angle;
    angle.SetAsRadian(radian);
    return angle;
  }

  static constexpr Angle FromDegree(Degree degree) {
    Angle angle;
    angle.SetAsDegree(degree);
    return angle;
  }

  constexpr void SetAsRadian(Rad radian) {
    radian_ = radian;
  }

  constexpr void SetAsDegree(Degree degree) {
    radian_ = degree * (kPi / 180.0f);
  }

  [[nodiscard]] constexpr Rad GetAsRadian() const {
    return radian_;
  }

  [[nodiscard]] constexpr Degree GetAsDegree() const {
    return radian_ * (180.0f / kPi);
  }

 private:
  Rad radian_ = 0.0f;  // 角度 [rad]
};

namespace literals {

constexpr Velocity operator""_rpm(long double value) {
  return Velocity::FromRpm(static_cast<Rpm>(value));
}

constexpr Velocity operator""_rad_ps(long double value) {
  return Velocity::FromRadPS(static_cast<RadPS>(value));
}

constexpr Current operator""_A(long double value) {
  return Current::FromAmpere(static_cast<float>(value));
}

constexpr Current operator""_mA(long double value) {
  return Current::FromMilliAmpere(static_cast<float>(value));
}

constexpr Angle operator""_deg(long double value) {
  return Angle::FromDegree(static_cast<Degree>(value));
}

constexpr Angle operator""_rad(long double value) {
  return Angle::FromRadian(static_cast<Rad>(value));
}

}  // namespace literals

}  // namespace nbed
