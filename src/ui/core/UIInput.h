#pragma once

#include "../../input.h"
#include "UITypes.h"
#include <SDL_mouse.h>
#include <SDL_rect.h>
#include <SDL_stdinc.h>

namespace UI {

// Per-frame mouse snapshot. Fill it once per frame, AFTER Input::update().
struct MouseState {
  int x = 0;
  int y = 0;
  bool down = false;     // button held this frame
  bool pressed = false;  // button went down this frame
  bool released = false; // button went up this frame
};

inline MouseState readMouse(Uint8 button = SDL_BUTTON_LEFT) {
  MouseState m;
  const Vector2i pos = Input::getMousePos();
  m.x = pos.x;
  m.y = pos.y;
  m.down = Input::isMouseButtonHeld(button);
  m.pressed = Input::isMouseButtonPressed(button);
  m.released = Input::isMouseButtonReleased(button);
  return m;
}

inline bool contains(const SDL_FRect &r, int x, int y) {
  const float fx = static_cast<float>(x);
  const float fy = static_cast<float>(y);
  return fx >= r.x && fx < r.x + r.w && fy >= r.y && fy < r.y + r.h;
}

// Click logic shared by both UI styles. Immediate mode keeps one per ID,
// retained mode keeps one per widget.
struct PressState {
  bool hover = false;   // cursor is over the area
  bool down = false;    // a press STARTED inside and the button is still held
  bool clicked = false; // true for exactly one frame: released while still over
};

// Hit-test against the widget's BASE (layout) rect, not its animated draw rect,
// otherwise a squish or grow animation feeds back into hover and flickers.
// Overlapping widgets are not resolved: every widget under the cursor reacts.
inline void updatePress(PressState &s, const SDL_FRect &area,
                        const MouseState &m) {
  s.clicked = false;
  s.hover = contains(area, m.x, m.y);

  if (m.pressed && s.hover)
    s.down = true;

  if (m.released) {
    if (s.down && s.hover)
      s.clicked = true;
    s.down = false;
  }

  if (!m.down) // e.g. released outside the window
    s.down = false;
}

inline void updatePress(PressState &s, const SDL_Rect &area,
                        const MouseState &m) {
  updatePress(s, toF(area), m);
}

} // namespace UI
