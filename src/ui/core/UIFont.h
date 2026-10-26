#pragma once

#include "font.h"
#include <SDL_log.h>
#include <SDL_stdinc.h>
#include <SDL_ttf.h>
#include <cassert>
#include <cstddef>
#include <unordered_map>

// A TTF_Font is tied to ONE size, so the cache is keyed by (FontID, size).
// Lookups only happen when text is (re)rendered, never per frame.
namespace UI::Font {

inline std::unordered_map<Uint64, TTF_Font *> cache;

inline constexpr Uint64 makeKey(FontID id, int size) {
  return (static_cast<Uint64>(id) << 32) | static_cast<Uint32>(size);
}

// Opens the font on first request. Returns nullptr (and logs) on failure.
// Failures are not cached, so a bad path logs again on the next text render.
inline TTF_Font *get(FontID id, int size) {
  assert(static_cast<size_t>(id) < static_cast<size_t>(FontID::Count));
  assert(size > 0);

  const Uint64 key = makeKey(id, size);
  if (auto it = cache.find(key); it != cache.end())
    return it->second;

  const char *path = FontPaths[static_cast<size_t>(id)];
  TTF_Font *font = TTF_OpenFont(path, size);
  if (!font) {
    SDL_Log("Failed to open font '%s' (size %d): %s", path, size,
            TTF_GetError());
    return nullptr;
  }
  cache.emplace(key, font);
  return font;
}

// Call before TTF_Quit().
inline void kill() {
  for (auto &entry : cache)
    TTF_CloseFont(entry.second);
  cache.clear();
}

} // namespace UI::Font
