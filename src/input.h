#pragma once

#include "vector2.hpp"
#include <SDL.h>
#include <unordered_map>

namespace Input {

inline const Uint8 *keyboardState = nullptr;
inline std::unordered_map<SDL_Scancode, bool> prevKeyState = {};

inline Vector2i mousePos = {0, 0};
inline Uint32 mouseState = 0;
inline Uint32 prevMouseState = 0;

// Captures the current keyboard and mouse state. Call once per frame before
// any input queries.
inline void update() {
  keyboardState = SDL_GetKeyboardState(nullptr);

  int x, y;
  mouseState = SDL_GetMouseState(&x, &y);
  mousePos = {x, y};
}

// Call at the VERY END of the frame, after gameplay code has read input.
// This snapshots "current" into "previous" for next frame's edge detection.
inline void lateUpdate() {
  for (auto &pair : prevKeyState) {
    pair.second = keyboardState[pair.first];
  }
  prevMouseState = mouseState;
}

// ---------------- Keyboard ----------------

// True while the key is held down.
inline bool isKeyHeld(SDL_Scancode key) {
  if (!keyboardState)
    return false;
  if (prevKeyState.find(key) == prevKeyState.end()) {
    prevKeyState[key] = false;
  }
  return keyboardState[key];
}

// True only on the frame the key goes from up to down.
inline bool isKeyPressed(SDL_Scancode key) {
  if (!keyboardState)
    return false;
  bool prev = prevKeyState.count(key) ? prevKeyState[key] : false;
  bool curr = keyboardState[key];
  if (!prevKeyState.count(key))
    prevKeyState[key] = false;
  return curr && !prev;
}

// True only on the frame the key goes from down to up.
inline bool isKeyReleased(SDL_Scancode key) {
  if (!keyboardState)
    return false;
  bool prev = prevKeyState.count(key) ? prevKeyState[key] : false;
  bool curr = keyboardState[key];
  if (!prevKeyState.count(key))
    prevKeyState[key] = false;
  return !curr && prev;
}

// ---------------- Mouse ----------------

// Mouse position captured by the last update().
inline Vector2i getMousePos() {
  return mousePos;
}

// True while the mouse button is held (button is an SDL_BUTTON_* value).
inline bool isMouseButtonHeld(Uint8 button) {
  return mouseState & SDL_BUTTON(button);
}

// True only on the frame the mouse button is pressed.
inline bool isMouseButtonPressed(Uint8 button) {
  bool curr = mouseState & SDL_BUTTON(button);
  bool prev = prevMouseState & SDL_BUTTON(button);
  return curr && !prev;
}

// True only on the frame the mouse button is released.
inline bool isMouseButtonReleased(Uint8 button) {
  bool curr = mouseState & SDL_BUTTON(button);
  bool prev = prevMouseState & SDL_BUTTON(button);
  return !curr && prev;
}

}; // namespace Input
