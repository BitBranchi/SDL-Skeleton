#pragma once

#include "vector2.hpp"
#include <SDL.h>
#include <unordered_map>

namespace Input {

const Uint8 *keyboardState = nullptr;
std::unordered_map<SDL_Scancode, bool> prevKeyState = {};

Vector2i mousePos = {0, 0};
Uint32 mouseState = 0;
Uint32 prevMouseState = 0;

void update() {
  keyboardState = SDL_GetKeyboardState(nullptr);

  int x, y;
  mouseState = SDL_GetMouseState(&x, &y);
  mousePos = {x, y};
}

// Call at the VERY END of the frame, after gameplay code has read input.
// This snapshots "current" into "previous" for next frame's edge detection.
void lateUpdate() {
  for (auto &pair : prevKeyState) {
    pair.second = keyboardState[pair.first];
  }
  prevMouseState = mouseState;
}

// ---------------- Keyboard ----------------

bool isKeyHeld(SDL_Scancode key) {
  if (!keyboardState)
    return false;
  if (prevKeyState.find(key) == prevKeyState.end()) {
    prevKeyState[key] = false;
  }
  return keyboardState[key];
}

bool isKeyPressed(SDL_Scancode key) {
  if (!keyboardState)
    return false;
  bool prev = prevKeyState.count(key) ? prevKeyState[key] : false;
  bool curr = keyboardState[key];
  if (!prevKeyState.count(key))
    prevKeyState[key] = false;
  return curr && !prev;
}

bool isKeyReleased(SDL_Scancode key) {
  if (!keyboardState)
    return false;
  bool prev = prevKeyState.count(key) ? prevKeyState[key] : false;
  bool curr = keyboardState[key];
  if (!prevKeyState.count(key))
    prevKeyState[key] = false;
  return !curr && prev;
}

// ---------------- Mouse ----------------

Vector2i getMousePos() {
  return mousePos;
}

bool isMouseButtonHeld(Uint8 button) {
  return mouseState & SDL_BUTTON(button);
}

bool isMouseButtonPressed(Uint8 button) {
  bool curr = mouseState & SDL_BUTTON(button);
  bool prev = prevMouseState & SDL_BUTTON(button);
  return curr && !prev;
}

bool isMouseButtonReleased(Uint8 button) {
  bool curr = mouseState & SDL_BUTTON(button);
  bool prev = prevMouseState & SDL_BUTTON(button);
  return !curr && prev;
}

}; // namespace Input
