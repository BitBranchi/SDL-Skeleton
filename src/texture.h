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

  Count,
};

inline constexpr std::array Paths = {
    "assets/no_texture.png",     // NoTexture
    UI_PATH "panel_bright.png",  // PanelBright
    UI_PATH "panel_dark.png",    // PanelDark
    UI_PATH "button_bright.png", // ButtonBright
    UI_PATH "button_dark.png",   // ButtonDark
};

static_assert(Paths.size() == static_cast<size_t>(TextureID::Count),
              "Paths must have one entry per TextureID");

#undef UI_PATH

#endif
