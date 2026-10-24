#pragma once

#include <SDL_rect.h>
#include <cmath>

namespace UI {

inline SDL_FRect toF(const SDL_Rect &r) {
  return SDL_FRect{static_cast<float>(r.x), static_cast<float>(r.y),
                   static_cast<float>(r.w), static_cast<float>(r.h)};
}

// Rounds the EDGES of a rect (left/top and right/bottom), not its size.
// Two float rects that share an edge still share it after rounding, so
// nine-slice pieces never get a 1px seam between them.
inline SDL_Rect snap(const SDL_FRect &r) {
  const int x0 = static_cast<int>(std::lround(r.x));
  const int y0 = static_cast<int>(std::lround(r.y));
  const int x1 = static_cast<int>(std::lround(r.x + r.w));
  const int y1 = static_cast<int>(std::lround(r.y + r.h));
  return SDL_Rect{x0, y0, x1 - x0, y1 - y0};
}

} // namespace UI
