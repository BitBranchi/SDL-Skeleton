#pragma once

#include "texture.h"
#include <SDL_image.h>
#include <SDL_log.h>
#include <SDL_render.h>
#include <cassert>
#include <vector>

namespace ResourceManager {

inline SDL_Renderer *tRenderer = nullptr;
inline std::vector<SDL_Texture *> cache = {};

inline SDL_Rect panelRect[9];
inline bool panelRectReady = false;

// Config
inline constexpr int PANEL_BORDER = 48;

// Must run this function first to initialize reosurce manager.
inline void init(SDL_Renderer *renderer) {
  if (!renderer) {
    SDL_Log("ResourceManager::init called with null renderer");
    return;
  }
  tRenderer = renderer;
  cache.assign(static_cast<size_t>(TextureID::Count), nullptr);
}

// Loads on first request, cached afterwards. Fetch once and store the pointer
// rather than calling this every frame.
inline SDL_Texture *get(TextureID ID) {
  const size_t index = static_cast<size_t>(ID);
  assert(index < static_cast<size_t>(TextureID::Count));

  SDL_Texture *&slot = cache[index];
  if (slot)
    return slot;

  slot = IMG_LoadTexture(tRenderer, TexturePaths[index]);
  if (!slot) {
    SDL_Log("Failed to load '%s': %s", TexturePaths[index], IMG_GetError());
    slot = IMG_LoadTexture(
        tRenderer, TexturePaths[static_cast<size_t>(TextureID::NoTexture)]);
  }
  return slot;
}

// Call once, after tRenderer is set (or lazily before the first panel/button
// is drawn).
inline void ensurePanelRect(TextureID ID = TextureID::PanelBright) {
  if (panelRectReady)
    return;

  SDL_Texture *tex = get(ID);
  if (!tex)
    return; // even the fallback failed; avoid querying a null texture

  int w = 0, h = 0;
  SDL_QueryTexture(tex, NULL, NULL, &w, &h);

  int right = w - PANEL_BORDER;
  int bottom = h - PANEL_BORDER;

  panelRect[0] = {0, 0, w, h};                                // center
  panelRect[1] = {0, 0, w, PANEL_BORDER};                     // top edge
  panelRect[2] = {0, 0, PANEL_BORDER, h};                     // left edge
  panelRect[3] = {right, 0, PANEL_BORDER, h};                 // right edge
  panelRect[4] = {0, bottom, w, PANEL_BORDER};                // bottom edge
  panelRect[5] = {0, 0, PANEL_BORDER, PANEL_BORDER};          // top-left
  panelRect[6] = {right, 0, PANEL_BORDER, PANEL_BORDER};      // top-right
  panelRect[7] = {0, bottom, PANEL_BORDER, PANEL_BORDER};     // bottom-left
  panelRect[8] = {right, bottom, PANEL_BORDER, PANEL_BORDER}; // bottom-right

  panelRectReady = true;
}

// This should be called only when the game is closing, before the renderer
// is destroyed.
inline void kill() {
  for (SDL_Texture *tex : cache) {
    if (tex)
      SDL_DestroyTexture(tex);
  }
  cache.clear();
  panelRectReady = false;
  tRenderer = nullptr;
}

} // namespace ResourceManager
