#pragma once

#include "ImState.h"
#include <SDL_rect.h>
#include <algorithm>
#include <cmath>
#include <string_view>

// Widget IDs are plain ints you choose (an enum per screen works well).
// They must be UNIQUE across every widget type, because they share one state map.

namespace UI {

enum class Align { Left, Center, Right };

namespace Im {

// Draws a text texture inside a rect, vertically centered.
inline void drawText(const TextTexture &tt, const SDL_Rect &r, Align align) {
  const float w = static_cast<float>(tt.width());
  const float h = static_cast<float>(tt.height());
  float x = static_cast<float>(r.x);
  if (align == Align::Center)
    x = r.x + (r.w - w) * 0.5f;
  else if (align == Align::Right)
    x = r.x + r.w - w;
  const float y = r.y + (r.h - h) * 0.5f;
  Draw::text(tt, x, y);
}

} // namespace Im

// ---------------- Stateless ----------------

inline void panel(const SDL_Rect &r) {
  Draw::nineSlice(theme().panel, toF(r));
}

inline void image(TextureID id, const SDL_Rect &r) {
  Draw::texture(id, toF(r));
}

// fraction: 0..1. Animate it yourself with a UI::Tween if you want it smooth.
inline void progressBar(const SDL_Rect &r, float fraction) {
  const Theme &th = theme();
  const float f = std::clamp(fraction, 0.0f, 1.0f);
  Draw::nineSlice(th.track, toF(r));
  if (f > 0.0f) {
    Draw::nineSlice(th.fill,
                    SDL_FRect{static_cast<float>(r.x), static_cast<float>(r.y),
                              r.w * f, static_cast<float>(r.h)});
  }
}

// ---------------- Label ----------------
// Text is rendered once and cached under id; it re-renders only when the
// string (or theme font/size/color) changes. wrap = wrap lines at r.w.
// Wrapped lines are left-aligned inside their block.
inline void label(int id, const SDL_Rect &r, std::string_view text,
                  Align align = Align::Center, bool wrap = false) {
  if (text.empty())
    return;
  Im::WidgetState &s = Im::states[id];
  const TextTexture &tt = Im::ensureText(s, text, wrap ? r.w : 0);
  Im::drawText(tt, r, align);
}

// ---------------- Button ----------------
// Returns true on the frame it is clicked (press AND release inside).
inline bool button(int id, const SDL_Rect &r, std::string_view text,
                   bool enabled = true) {
  const Theme &th = theme();
  Im::WidgetState &s = Im::interact(id, r, enabled);

  // Wider and shorter while hovered, and drops a little while pressed.
  const float grow = th.squish * s.hoverT.value();
  const float drop = th.pressOffset * s.pressT.value();
  const SDL_FRect box{r.x - grow * 0.5f, r.y + grow * 0.5f + drop, r.w + grow,
                      r.h - grow};

  const Draw::Scope dim(
      Draw::Transform{0.0f, 0.0f, enabled ? 1.0f : th.disabledAlpha});
  const bool lit = s.press.hover || s.press.down;
  Draw::nineSlice(lit ? th.buttonHover : th.button, box);
  if (!text.empty())
    Draw::textCentered(Im::ensureText(s, text, 0), box);

  return s.press.clicked;
}

// ---------------- Toggle ----------------
// A square box (side = r.h) with optional text to its right. The whole rect is
// clickable. Returns true on the frame the value changed.
inline bool toggle(int id, const SDL_Rect &r, bool &value,
                   std::string_view text = {}, bool enabled = true) {
  const Theme &th = theme();
  Im::WidgetState &s = Im::interact(id, r, enabled);

  bool changed = false;
  if (s.press.clicked) {
    value = !value;
    changed = true;
  }

  const float grow = th.squish * 0.5f * s.hoverT.value();
  const float side = static_cast<float>(r.h);
  const SDL_FRect box{r.x - grow * 0.5f, r.y - grow * 0.5f, side + grow,
                      side + grow};

  const Draw::Scope dim(
      Draw::Transform{0.0f, 0.0f, enabled ? 1.0f : th.disabledAlpha});
  Draw::nineSlice(value ? th.toggleOn : th.toggleOff, box);

  if (!text.empty()) {
    const int gap = 8;
    const SDL_Rect textArea{r.x + r.h + gap, r.y, r.w - r.h - gap, r.h};
    Im::drawText(Im::ensureText(s, text, 0), textArea, Align::Left);
  }
  return changed;
}

// ---------------- Slider ----------------
// Click or drag anywhere on the rect. The drag keeps following the mouse after
// it leaves the rect, until the button is released. step > 0 snaps the value.
// Returns true on frames where the value changed.
inline bool slider(int id, const SDL_Rect &r, float &value, float minV = 0.0f,
                   float maxV = 1.0f, float step = 0.0f, bool enabled = true) {
  const Theme &th = theme();
  Im::WidgetState &s = Im::interact(id, r, enabled);
  const Draw::Transform xf = Draw::current();

  const float span = maxV - minV;
  const int handleW = std::clamp(r.h, 1, std::max(1, r.w));
  const float half = static_cast<float>(handleW) * 0.5f;
  const float usable = static_cast<float>(r.w - handleW);

  bool changed = false;
  if (s.press.down && span > 0.0f && usable > 0.0f) {
    // Mouse is in screen space, so undo the current transform offset.
    const float left = static_cast<float>(r.x) + xf.x + half;
    float t = (static_cast<float>(frame.mouse.x) - left) / usable;
    t = std::clamp(t, 0.0f, 1.0f);
    float v = minV + t * span;
    if (step > 0.0f)
      v = minV + std::round((v - minV) / step) * step;
    v = std::clamp(v, minV, maxV);
    if (v != value) {
      value = v;
      changed = true;
    }
  }

  const float t =
      span > 0.0f ? std::clamp((value - minV) / span, 0.0f, 1.0f) : 0.0f;
  const float cx = static_cast<float>(r.x) + half + usable * t; // handle center

  const float trackH = std::max(4.0f, static_cast<float>(r.h) * 0.4f);
  const SDL_FRect track{static_cast<float>(r.x),
                        r.y + (static_cast<float>(r.h) - trackH) * 0.5f,
                        static_cast<float>(r.w), trackH};

  const Draw::Scope dim(
      Draw::Transform{0.0f, 0.0f, enabled ? 1.0f : th.disabledAlpha});
  Draw::nineSlice(th.track, track);

  const float fillW = cx - static_cast<float>(r.x);
  if (fillW > 0.0f)
    Draw::nineSlice(th.fill, SDL_FRect{track.x, track.y, fillW, trackH});

  const float grow = th.squish * s.hoverT.value();
  const SDL_FRect handle{cx - half - grow * 0.5f, r.y - grow * 0.5f,
                         static_cast<float>(handleW) + grow,
                         static_cast<float>(r.h) + grow};
  Draw::nineSlice(th.handle, handle);

  return changed;
}

} // namespace UI
