#ifndef SOUND_H
#define SOUND_H

#include <array>
#include <cstddef>

#define SOUND_PATH "assets/sound/"
#define MUSIC_PATH "assets/music/"

//-------- Musics --------//
enum class MusicID : size_t {
  SHUTTER,
  Count,
};
inline constexpr std::array MusicPaths = {
    MUSIC_PATH "shutter.ogg", // SHUTTER
};
static_assert(MusicPaths.size() == static_cast<size_t>(MusicID::Count),
              "Paths must have one entry per MusicID");

//-------- Sounds --------//
enum class SoundID : size_t {
  SHUTTER,
  Count,
};
inline constexpr std::array SoundPaths = {
    SOUND_PATH "shutter.ogg", // SHUTTER
};
static_assert(SoundPaths.size() == static_cast<size_t>(SoundID::Count),
              "Paths must have one entry per SoundID");

#undef SOUND_PATH
#undef MUSIC_PATH

#endif
