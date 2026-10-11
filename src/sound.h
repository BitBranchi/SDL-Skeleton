#ifndef SOUND_H
#define SOUND_H

#include <array>
#include <cstddef>

#define SOUND_PATH "assets/sound/"

enum class SoundID : size_t {
  SHUTTER,

  Count,
};

inline constexpr std::array Paths = {
    SOUND_PATH "shutter.ogg", // SHUTTER
};

static_assert(Paths.size() == static_cast<size_t>(SoundID::Count),
              "Paths must have one entry per SoundID");

#undef SOUND_PATH

#endif
