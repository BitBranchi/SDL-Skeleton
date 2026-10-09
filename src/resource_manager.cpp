#pragma once

#include "texture.h"
#include <SDL_image.h>
#include <SDL_log.h>
#include <SDL_render.h>
#include <string>
#include <unordered_map>

namespace ResourceManager {

inline SDL_Renderer *tRenderer = nullptr;
inline std::unordered_map<std::string, SDL_Texture *> cache;
inline SDL_Texture *missingTex = nullptr; // shared fallback, owned separately

inline SDL_Rect panelRect[9];
inline bool panelRectReady = false;
inline constexpr int PANEL_BORDER = 48;

// Loads on first request, cached afterwards. Fetch once and store the pointer
// rather than calling this every frame.
inline SDL_Texture *get(const std::string &path) {
  auto it = cache.find(path);
  if (it != cache.end())
    return it->second;

  SDL_Texture *tex = IMG_LoadTexture(tRenderer, path.c_str());
  if (tex) {
    cache.emplace(path, tex);
    return tex;
  }

  SDL_Log("Failed to load '%s': %s", path.c_str(), IMG_GetError());

  // Failures are not cached, so a bad path retries (and logs) every time.
  if (!missingTex) {
    missingTex = IMG_LoadTexture(tRenderer, Texture::NO_TEXTURE);
    if (!missingTex)
      SDL_Log("Fallback texture failed too: %s", IMG_GetError());
  }
  return missingTex; // may still be null if the fallback is missing
}

// Call once, after tRenderer is set (or lazily before the first panel/button
// is drawn).
inline void ensurePanelRect(const std::string &sourcePath = Texture::PANEL_BRIGHT) {
  if (panelRectReady)
    return;

  SDL_Texture *tex = get(sourcePath);
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
  for (auto &item : cache)
    SDL_DestroyTexture(item.second);
  cache.clear();
  SDL_DestroyTexture(missingTex); // safe on nullptr
  missingTex = nullptr;
  panelRectReady = false;
  tRenderer = nullptr;
}

} // namespace ResourceManager
