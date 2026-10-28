#pragma once

#include "../../texture.h"
#include "font.h"
#include <SDL_render.h>

namespace UI {

// Shared nine-slice corner size in source pixels for every theme.
inline constexpr int uiBorder = 8;

// A texture drawn as a nine-slice: corners keep their size, edges and center
// stretch. border is the corner size in source pixels.
struct NineSlice {
  TextureID texture = TextureID::NoTexture;
  int border = uiBorder;
};

struct Theme {
  // ---- Looks (all nine-slices) ----
  NineSlice panel;
  NineSlice button;
  NineSlice buttonHover; // also used while the button is held down
  NineSlice track;       // slider track, progress bar background
  NineSlice fill;        // slider filled part, progress bar fill
  NineSlice handle;      // slider handle
  NineSlice toggleOff;
  NineSlice toggleOn;

  // ---- Text ----
  FontID font = FontID::Main;
  int fontSize = 24;
  SDL_Color textColor{255, 255, 255, 255};

  // ---- Feel ----
  float squish = 6.0f;        // px a hovered button grows wider / shrinks shorter
  float pressOffset = 2.0f;   // px a pressed button drops
  float animSeconds = 0.15f;  // hover / press animation length
  float disabledAlpha = 0.5f; // disabled widgets are drawn this transparent
};

namespace Themes {

// Only the existing panel/button textures are used, so no new art is needed.
// Give the slider/toggle/progress fields their own textures when you have them.
inline constexpr Theme makeDark() {
  Theme t;
  t.panel = {TextureID::PanelDark, uiBorder};
  t.button = {TextureID::ButtonDark, uiBorder};
  t.buttonHover = {TextureID::ButtonBright, uiBorder};
  t.track = {TextureID::PanelDark, uiBorder};
  t.fill = {TextureID::FillBright, uiBorder};
  t.handle = {TextureID::KnobBright, uiBorder};
  t.toggleOff = {TextureID::ButtonDark, uiBorder};
  t.toggleOn = {TextureID::CheckBright, uiBorder};
  return t;
}

inline constexpr Theme makeBright() {
  Theme t;
  t.panel = {TextureID::PanelBright, uiBorder};
  t.button = {TextureID::ButtonBright, uiBorder};
  t.buttonHover = {TextureID::ButtonDark, uiBorder};
  t.track = {TextureID::PanelBright, uiBorder};
  t.fill = {TextureID::FillBright, uiBorder};
  t.handle = {TextureID::KnobBright, uiBorder};
  t.toggleOff = {TextureID::ButtonDark, uiBorder};
  t.toggleOn = {TextureID::CheckBright, uiBorder};
  return t;
}

inline constexpr Theme Dark = makeDark();
inline constexpr Theme Bright = makeBright();

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
