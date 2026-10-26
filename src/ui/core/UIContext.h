#pragma once

#include "UIFont.h"
#include <SDL_log.h>
#include <SDL_render.h>

namespace UI {

inline SDL_Renderer *renderer = nullptr;

// Call once, after SDL_CreateRenderer, TTF_Init and ResourceManager::init.
inline void init(SDL_Renderer *r) {
  if (!r) {
    SDL_Log("UI::init called with null renderer");
    return;
  }
  renderer = r;
}

// Call BEFORE SDL_DestroyRenderer and TTF_Quit.
// Closes cached fonts and marks the renderer as gone, so TextTexture objects
// destroyed later don't touch textures SDL already freed with the renderer.
inline void kill() {
  Font::kill();
  renderer = nullptr;
}

inline bool ready() {
  return renderer != nullptr;
}

} // namespace UI
