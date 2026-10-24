#pragma once

#include "../texture.h"
#include "font.h"
#include <SDL_pixels.h>

namespace UI {

// A texture drawn as a nine-slice: corners keep their size, edges and center
// stretch. border is the corner size in source pixels.
struct NineSlice {
  TextureID texture = TextureID::NoTexture;
  int border = 48;
};

struct Theme {
  NineSlice panel;
  NineSlice button;
  NineSlice buttonHover;
  FontID font = FontID::Main;
  int fontSize = 24;
  SDL_Color textColor{255, 255, 255, 255};
};

namespace Themes {

inline constexpr Theme Dark{
    {TextureID::PanelDark, 48},
    {TextureID::ButtonDark, 48},
    {TextureID::ButtonBright, 48},
    FontID::Main,
    24,
    {255, 255, 255, 255}};

inline constexpr Theme Bright{
    {TextureID::PanelBright, 48},
    {TextureID::ButtonBright, 48},
    {TextureID::ButtonDark, 48},
    FontID::Main,
    24,
    {255, 255, 255, 255}};

} // namespace Themes

inline const Theme *active = &Themes::Dark;

// The theme must outlive its use (a global or constexpr, not a local).
inline void setTheme(const Theme &t) {
  active = &t;
}

inline const Theme &theme() {
  return *active;
}

} // namespace UI
