#pragma once

#include <algorithm>

namespace UI {

enum class Ease {
  Linear,
  InQuad,
  OutQuad,
  InOutQuad,
  OutCubic,
  Overshoot, // the curve from the old ui.h: peaks at about 1.25, settles at 1
};

inline float ease(Ease e, float t) {
  t = std::clamp(t, 0.0f, 1.0f);
  switch (e) {
  case Ease::Linear:
    return t;
  case Ease::InQuad:
    return t * t;
  case Ease::OutQuad:
    return 1.0f - (1.0f - t) * (1.0f - t);
  case Ease::InOutQuad:
    return t < 0.5f ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
  case Ease::OutCubic: {
    const float u = 1.0f - t;
    return 1.0f - u * u * u;
  }
  case Ease::Overshoot:
    return t * (3.6f - 2.6f * t);
  }
  return t;
}

inline float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

// One mechanism for every animation:
//   hover/press : setTarget(isHovered) each frame, use value() as 0..1
//   slide-in    : setTarget(true) once, lerp positions with value()
// progress moves toward target at 1/duration per second; easing is applied on
// read, so going back (target 0) retraces the same curve.
struct Tween {
  float progress = 0.0f; // raw 0..1
  float target = 0.0f;   // 0 or 1
  float duration = 0.2f; // seconds for a full 0 -> 1 trip
  Ease easing = Ease::OutQuad;

  Tween() = default;
  explicit Tween(float seconds, Ease e = Ease::OutQuad)
      : duration(seconds), easing(e) {}

  void setTarget(bool on) { target = on ? 1.0f : 0.0f; }

  // Skip the animation and sit at an end.
  void jumpTo(bool on) {
    target = on ? 1.0f : 0.0f;
    progress = target;
  }

  void update(float dt) {
    if (progress == target)
      return;
    const float step = duration > 0.0f ? dt / duration : 1.0f;
    progress = progress < target ? std::min(progress + step, target)
                                 : std::max(progress - step, target);
  }

  float value() const { return ease(easing, progress); }
  bool done() const { return progress == target; }
};

} // namespace UI
