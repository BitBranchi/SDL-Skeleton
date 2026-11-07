#pragma once

#include "ui/core/UITypes.h"
#include "util/vector2.hpp"

#include <SDL_rect.h>
#include <SDL_render.h>
#include <algorithm>
#include <cmath>

// Camera state + behavior. World data (positions, physics) is never touched;
// the camera only decides which part of the world ends up on screen.
struct Camera {
  Vector2f target{};         // world point to follow
  Vector2f pos{};            // smoothed camera center, world space
  Vector2f deadzone{80, 60}; // half-size of the "don't move" box
  Vector2f worldMin{};       // level bounds; worldMin == worldMax disables
  Vector2f worldMax{};
  Vector2f viewSize{1280, 720}; // screen size in pixels
  float zoom = 1.0f;
  float follow = 6.0f; // higher = snappier

  // Smooths pos toward target. Call once per frame, after gameplay moved.
  void update(float dt) {
    Vector2f goal = pos;
    if (target.x > pos.x + deadzone.x)
      goal.x = target.x - deadzone.x;
    else if (target.x < pos.x - deadzone.x)
      goal.x = target.x + deadzone.x;
    if (target.y > pos.y + deadzone.y)
      goal.y = target.y - deadzone.y;
    else if (target.y < pos.y - deadzone.y)
      goal.y = target.y + deadzone.y;

    // Framerate-independent; lerp(pos, goal, 0.1f) would depend on fps.
    const float k = 1.0f - std::exp(-follow * dt);
    pos = Vector2f::lerp(pos, goal, k);
  }
};

// Per-frame snapshot. All world->screen conversion goes through here so no
// other code has to know about the camera.
struct CameraView {
  Vector2f origin{};  // world position of the top-left screen corner
  Vector2f size{};    // screen size in pixels
  Vector2f visible{}; // visible world size, cached for culling
  float zoom = 1.0f;

  CameraView() = default;
  CameraView(const Camera &cam) : zoom(cam.zoom) {
    size = cam.viewSize;
    origin = cam.pos - size / (2.0f * zoom);

    if (cam.worldMax != cam.worldMin) {
      const Vector2f maxOrigin = cam.worldMax - size / zoom;
      if (maxOrigin.x > cam.worldMin.x)
        origin.x = std::clamp(origin.x, cam.worldMin.x, maxOrigin.x);
      if (maxOrigin.y > cam.worldMin.y)
        origin.y = std::clamp(origin.y, cam.worldMin.y, maxOrigin.y);
    }

    visible = size / zoom;
  }

  SDL_FRect toScreen(const SDL_FRect &w) const {
    return {(w.x - origin.x) * zoom, (w.y - origin.y) * zoom, w.w * zoom,
            w.h * zoom};
  }
  Vector2f toScreen(Vector2f p) const { return (p - origin) * zoom; }
  Vector2f toWorld(Vector2f p) const { return origin + p / zoom; }

  bool visibleRect(const SDL_FRect &w) const {
    return w.x + w.w > origin.x && w.x < origin.x + visible.x &&
           w.y + w.h > origin.y && w.y < origin.y + visible.y;
  }
};

// The only draw path for world objects: cull, transform, snap, draw.
// pixelSnap = false keeps sub-pixel motion; true avoids shimmer on pixel art.
inline void drawSprite(SDL_Renderer *r, SDL_Texture *tex, const SDL_Rect *src,
                       const SDL_FRect &world, const CameraView &view,
                       bool pixelSnap = true) {
  if (!tex || !view.visibleRect(world))
    return;

  const SDL_FRect d = view.toScreen(world);
  SDL_Rect out;
  if (pixelSnap) {
    out = UI::snap(d);
  } else {
    out = SDL_Rect{static_cast<int>(std::lround(d.x)),
                   static_cast<int>(std::lround(d.y)),
                   static_cast<int>(std::lround(d.w)),
                   static_cast<int>(std::lround(d.h))};
  }
  SDL_RenderCopy(r, tex, src, &out);
}
