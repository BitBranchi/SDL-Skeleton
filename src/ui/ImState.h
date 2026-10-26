#pragma once

#include "core/UICore.h"
#include <SDL_pixels.h>
#include <cassert>
#include <string>
#include <string_view>
#include <unordered_map>

namespace UI {

// ---------------- Per-frame data ----------------

struct FrameData {
  MouseState mouse;
  float dt = 0.0f;
};
inline FrameData frame;

// Call once at the start of the UI part of every frame.
//   UI::beginFrame(UI::readMouse(), dt);
inline void beginFrame(const MouseState &mouse, float dt) {
  frame.mouse = mouse;
  frame.dt = dt;
  Draw::beginFrame();
}

// Call once after all widgets for the frame. Catches a forgotten Draw::pop().
inline void endFrame() {
  assert(Draw::stack.size() == 1 &&
         "UI: unbalanced Draw::push / pop during this frame");
}

namespace Im {

// What a cached text texture was rendered with. Re-render only when it changes.
struct TextKey {
  std::string text;
  FontID font = FontID::Main;
  int size = 0;
  SDL_Color color{0, 0, 0, 0};
  int wrap = 0;
  bool valid = false;
};

// Everything a widget needs to remember between frames, found by its ID.
struct WidgetState {
  PressState press;
  bool hoverEntered = false; // true only on the frame the cursor arrived
  Tween hoverT;
  Tween pressT;
  TextTexture text;
  TextKey key;
};

inline std::unordered_map<int, WidgetState> states;

inline bool sameColor(SDL_Color a, SDL_Color b) {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

// Returns the widget's text texture, re-rendering it only if something changed.
inline const TextTexture &ensureText(WidgetState &s, std::string_view text,
                                     int wrap) {
  const Theme &th = theme();
  TextKey &k = s.key;

  const bool changed = !k.valid || std::string_view(k.text) != text ||
                       k.font != th.font || k.size != th.fontSize ||
                       !sameColor(k.color, th.textColor) || k.wrap != wrap;
  if (changed) {
    k.text.assign(text.data(), text.size());
    k.font = th.font;
    k.size = th.fontSize;
    k.color = th.textColor;
    k.wrap = wrap;
    k.valid = true; // even on failure, so a bad font logs once, not every frame
    s.text.set(k.text, th.font, th.fontSize, th.textColor, wrap);
  }
  return s.text;
}

// Hit-tests the widget and advances its animations.
// The hit area is the BASE rect moved by the current Draw transform, so a
// widget inside a sliding panel is clickable where it appears. Widgets that
// are disabled or fully transparent ignore the mouse.
inline WidgetState &interact(int id, const SDL_Rect &base, bool enabled) {
  const Theme &th = theme();
  const Draw::Transform xf = Draw::current();

  WidgetState &s = states[id];
  const bool live = enabled && xf.alpha > 0.01f;
  const bool wasHover = s.press.hover;

  if (live) {
    SDL_FRect area = toF(base);
    area.x += xf.x;
    area.y += xf.y;
    updatePress(s.press, area, frame.mouse);
  } else {
    s.press = PressState{};
  }
  s.hoverEntered = s.press.hover && !wasHover;

  s.hoverT.duration = th.animSeconds;
  s.hoverT.easing = Ease::Overshoot;
  s.pressT.duration = th.animSeconds;
  s.pressT.easing = Ease::OutQuad;
  s.hoverT.setTarget(s.press.hover || s.press.down);
  s.pressT.setTarget(s.press.down);
  s.hoverT.update(frame.dt);
  s.pressT.update(frame.dt);
  return s;
}

} // namespace Im

// ---------------- Queries (for sounds and other hooks) ----------------
// Valid after the widget with that ID has been called this frame.
//   if (UI::button(ID_PLAY, r, "Play")) startGame();
//   if (UI::hoverEntered(ID_PLAY)) SoundManager::play(SoundID::Hover);

inline bool isHovered(int id) {
  const auto it = Im::states.find(id);
  return it != Im::states.end() && it->second.press.hover;
}

inline bool isDown(int id) {
  const auto it = Im::states.find(id);
  return it != Im::states.end() && it->second.press.down;
}

inline bool hoverEntered(int id) {
  const auto it = Im::states.find(id);
  return it != Im::states.end() && it->second.hoverEntered;
}

// Forget all widget state (animations, cached text). Call on a screen change if
// you want animations to start fresh. Safe while the renderer still exists.
inline void clearState() {
  Im::states.clear();
}

} // namespace UI
