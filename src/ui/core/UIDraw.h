#pragma once

#include "../../resource_manager.h"
#include "Theme.h"
#include "UIContext.h"
#include "UIText.h"
#include "UITypes.h"
#include <SDL_render.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

namespace UI::Draw {

// Reusable buffers for nine-slice geometry
inline SDL_Vertex nineSliceVerts[16];
inline int nsIdx[54];
inline bool isNsInit = false;

inline void initNineSlice() {
  if (isNsInit)
    return;
  // fill the 54 indices in row-major order for 4x4 grid
  int k = 0;
  for (int gr = 0; gr < 3; ++gr) {
    for (int gc = 0; gc < 3; ++gc) {
      int base = gr * 4 + gc;
      nsIdx[k++] = base;
      nsIdx[k++] = base + 1;
      nsIdx[k++] = base + 4;
      nsIdx[k++] = base + 1;
      nsIdx[k++] = base + 5;
      nsIdx[k++] = base + 4;
    }
  }
  isNsInit = true;
}

// Offset + alpha applied to everything drawn while it is on the stack.
// Scale is NOT part of it: a widget that grows or squishes computes its own
// draw rect from its base rect.
struct Transform {
  float x = 0.0f;
  float y = 0.0f;
  float alpha = 1.0f; // 0..1
};

inline std::vector<Transform> stack{Transform{}};

inline const Transform &current() {
  return stack.back();
}

// Composes with the current top: offsets add, alphas multiply.
inline void push(const Transform &t) {
  const Transform top = stack.back();
  stack.push_back(Transform{top.x + t.x, top.y + t.y, top.alpha * t.alpha});
}

inline void pop() {
  assert(stack.size() > 1 && "UI::Draw::pop without a matching push");
  if (stack.size() > 1)
    stack.pop_back();
}

// Call at the start of each frame so a missing pop can't leak into the next.
inline void beginFrame() {
  stack.resize(1);
}

// push() now, pop() automatically at the end of the scope.
struct Scope {
  explicit Scope(const Transform &t) { push(t); }
  ~Scope() { pop(); }
  Scope(const Scope &) = delete;
  Scope &operator=(const Scope &) = delete;
};

// Applies the transform, snaps to whole pixels, handles alpha.
// exactSize: keep dst.w/dst.h as-is and only snap the position (for text, so
// a fractional position never stretches the glyphs by a pixel).
inline void copy(SDL_Texture *tex, const SDL_Rect *src, const SDL_FRect &dst,
                 bool exactSize = false) {
  if (!tex || !renderer)
    return;

  const Transform &t = current();
  const SDL_FRect moved{dst.x + t.x, dst.y + t.y, dst.w, dst.h};

  SDL_Rect out;
  if (exactSize) {
    out = SDL_Rect{static_cast<int>(std::lround(moved.x)),
                   static_cast<int>(std::lround(moved.y)),
                   static_cast<int>(moved.w), static_cast<int>(moved.h)};
  } else {
    out = snap(moved);
  }

  const float a = std::clamp(t.alpha, 0.0f, 1.0f);
  const Uint8 alpha = static_cast<Uint8>(a * 255.0f + 0.5f);
  if (alpha == 0)
    return;

  if (alpha != 255) {
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(tex, alpha);
  }
  SDL_RenderCopy(renderer, tex, src, &out);
  if (alpha != 255)
    SDL_SetTextureAlphaMod(tex, 255); // textures are shared through the cache
}

// A whole texture stretched into dst.
inline void texture(TextureID id, const SDL_FRect &dst) {
  copy(ResourceManager::get(id), nullptr, dst);
}

inline void nineSlice(const NineSlice &ns, const SDL_FRect &dst) {
  SDL_Texture *tex = ResourceManager::get(ns.texture);
  if (!tex)
    return;

  int w = 0;
  int h = 0;

  if (SDL_QueryTexture(tex, nullptr, nullptr, &w, &h) != 0 || w <= 0 || h <= 0)
    return;

  // Source border can't exceed half the texture; destination border can't
  // exceed half the destination (tiny panels shrink their corners).
  const int b = std::max(0, std::min({ns.border, w / 2, h / 2}));
  const float bx = std::min(static_cast<float>(b), dst.w * 0.5f);
  const float by = std::min(static_cast<float>(b), dst.h * 0.5f);
  // Destination gridlines (in dst space)
  const float gx[4] = {
      dst.x,
      dst.x + bx,
      dst.x + dst.w - bx,
      dst.x + dst.w};
  const float gy[4] = {
      dst.y,
      dst.y + by,
      dst.y + dst.h - by,
      dst.y + dst.h};

  // Source UVs in [0,1]
  const float ux[4] = {
      0.0f,
      (float)b / (float)w,
      (float)(w - b) / (float)w,
      1.0f};
  const float uy[4] = {
      0.0f,
      (float)b / (float)h,
      (float)(h - b) / (float)h,
      1.0f};

  const Transform &t = current();
  const float a = std::clamp(t.alpha, 0.0f, 1.0f);
  const Uint8 alpha = static_cast<Uint8>(a * 255.0f + 0.5f);
  if (alpha == 0)
    return;

  SDL_Color col{255, 255, 255, alpha};
  auto snapPt = [&](float x, float y) -> SDL_FPoint {
    return SDL_FPoint{static_cast<float>(std::lround(x)), static_cast<float>(std::lround(y))};
  };
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      const int vi = r * 4 + c;
      // transformed + snapped position
      const float px = gx[c] + t.x;
      const float py = gy[r] + t.y;
      const SDL_FPoint p = snapPt(px, py);

      nineSliceVerts[vi].position = p;
      nineSliceVerts[vi].tex_coord = SDL_FPoint{ux[c], uy[r]};
      nineSliceVerts[vi].color = col;
    }
  }
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
  SDL_RenderGeometry(renderer, tex, nineSliceVerts, 16, nsIdx, 54);
}

// Text at its native size, top-left at (x, y).
inline void text(const TextTexture &tt, float x, float y) {
  copy(tt.texture(), nullptr,
       SDL_FRect{x, y, static_cast<float>(tt.width()),
                 static_cast<float>(tt.height())},
       true);
}

inline void textCentered(const TextTexture &tt, const SDL_FRect &box) {
  const float x = box.x + (box.w - static_cast<float>(tt.width())) * 0.5f;
  const float y = box.y + (box.h - static_cast<float>(tt.height())) * 0.5f;
  text(tt, x, y);
}

} // namespace UI::Draw
