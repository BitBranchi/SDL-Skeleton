#ifndef FONT_H
#define FONT_H

#include <array>
#include <cstddef>

#define FONT_PATH "assets/font/"

enum class FontID : size_t {
  Main,

  Count,
};

inline constexpr std::array FontPaths = {
    FONT_PATH "font.ttf", // Main
};

static_assert(FontPaths.size() == static_cast<size_t>(FontID::Count),
              "FontPaths must have one entry per FontID");

#undef FONT_PATH

#endif
