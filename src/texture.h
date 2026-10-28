#ifndef TEXTURE_H
#define TEXTURE_H

#include <array>
#include <cstddef>

#define UI_PATH "assets/ui/"

enum class TextureID : size_t {
  NoTexture,

  // UI
  PanelBright,
  PanelDark,
  ButtonBright,
  ButtonDark,
  KnobBright,
  KnobDark,
  FillBright,
  FillDark,
  CheckBright,
  CheckDark,

  Count,
};

inline constexpr std::array TexturePaths = {
    "assets/no_texture.png",     // NoTexture
    UI_PATH "panel_bright.png",  // PanelBright
    UI_PATH "panel_dark.png",    // PanelDark
    UI_PATH "button_bright.png", // ButtonBright
    UI_PATH "button_dark.png",   // ButtonDark
    UI_PATH "knob_bright.png",   // KnobBright
    UI_PATH "knob_dark.png",     // KnobDark
    UI_PATH "fill_bright.png",   // FillBright
    UI_PATH "fill_dark.png",     // FillDark
    UI_PATH "check_bright.png",  // CheckBright
    UI_PATH "check_dark.png",    // CheckDark
};

static_assert(TexturePaths.size() == static_cast<size_t>(TextureID::Count),
              "Paths must have one entry per TextureID");

#undef UI_PATH

#endif
