#pragma once

#include "UIContext.h"
#include "UIFont.h"
#include <SDL_log.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_surface.h>
#include <SDL_ttf.h>
#include <string>

namespace UI {

enum class TextMode {
  Solid,   // aliased edges, crisp: suits pixel art
  Blended, // anti-aliased, smoother
};

// Text rendered ONCE into a texture. Call set() only when the text, font,
// size or color changes, never per frame. Move-only because it owns a texture.
class TextTexture {
public:
  TextTexture() = default;
  TextTexture(const TextTexture &) = delete;
  TextTexture &operator=(const TextTexture &) = delete;
  TextTexture(TextTexture &&other) noexcept { steal(other); }
  TextTexture &operator=(TextTexture &&other) noexcept {
    if (this != &other) {
      destroy();
      steal(other);
    }
    return *this;
  }
  ~TextTexture() { destroy(); }

  // wrapWidth: 0 = wrap only on '\n', otherwise wrap at that many pixels.
  // Wrapped text is left-aligned. Empty text is valid and draws nothing.
  // Returns false (and leaves the texture empty) on failure.
  bool set(const std::string &message, FontID fontId, int size,
           SDL_Color color, int wrapWidth = 0,
           TextMode mode = TextMode::Solid) {
    destroy();
    if (message.empty())
      return true;
    if (!renderer) {
      SDL_Log("TextTexture::set called before UI::init");
      return false;
    }

    TTF_Font *font = Font::get(fontId, size);
    if (!font)
      return false;

    const Uint32 wrap = wrapWidth > 0 ? static_cast<Uint32>(wrapWidth) : 0u;
    SDL_Surface *surface =
        (mode == TextMode::Solid)
            ? TTF_RenderText_Solid_Wrapped(font, message.c_str(), color, wrap)
            : TTF_RenderText_Blended_Wrapped(font, message.c_str(), color,
                                             wrap);
    if (!surface) {
      SDL_Log("Failed to render text: %s", TTF_GetError());
      return false;
    }

    tex = SDL_CreateTextureFromSurface(renderer, surface);
    const int surfaceW = surface->w;
    const int surfaceH = surface->h;
    SDL_FreeSurface(surface);
    if (!tex) {
      SDL_Log("Failed to create text texture: %s", SDL_GetError());
      return false;
    }

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    w = surfaceW;
    h = surfaceH;
    return true;
  }

  void destroy() {
    // If the renderer is already gone, SDL freed this texture along with it.
    if (tex && renderer)
      SDL_DestroyTexture(tex);
    tex = nullptr;
    w = 0;
    h = 0;
  }

  SDL_Texture *texture() const { return tex; }
  int width() const { return w; }
  int height() const { return h; }
  bool valid() const { return tex != nullptr; }

private:
  void steal(TextTexture &other) noexcept {
    tex = other.tex;
    w = other.w;
    h = other.h;
    other.tex = nullptr;
    other.w = 0;
    other.h = 0;
  }

  SDL_Texture *tex = nullptr;
  int w = 0;
  int h = 0;
};

} // namespace UI
