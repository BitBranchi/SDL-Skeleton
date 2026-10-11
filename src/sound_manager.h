#pragma once

#include "sound.h"
#include <SDL2/SDL_mixer.h>
#include <SDL_log.h>
#include <cassert>
#include <cstddef>
#include <vector>

namespace SoundManager {

// ---------------- Sound Effects (Mix_Chunk) ----------------
inline std::vector<Mix_Chunk *> cache = {};

// ---------------- Music (Mix_Music) ----------------
// Music is streamed rather than fully decoded into memory (unlike Mix_Chunk),
// and SDL_mixer only ever plays ONE music track at a time - that's a library
// limitation, not a choice made here.
inline std::vector<Mix_Music *> musicCache = {};
inline MusicID currentMusic;

// Must run this function before first get sound or music
inline void init() {
  cache.assign(static_cast<size_t>(SoundID::Count), nullptr);
  musicCache.assign(static_cast<size_t>(MusicID::Count), nullptr);
}

// This should be ran only when the game is closing
inline void kill() {
  for (auto &sound : cache) {
    if (sound)
      Mix_FreeChunk(sound);
  }
  cache.clear();
  for (auto &music : musicCache) {
    if (music)
      Mix_FreeMusic(music);
  }
  musicCache.clear();
}

// ---------------- Sound Effects ----------------

inline Mix_Chunk *get(SoundID ID) {
  const size_t index = static_cast<size_t>(ID);
  assert(index < static_cast<size_t>(SoundID::Count));
  Mix_Chunk *&slot = cache[index];

  if (slot) {
    return slot;
  }

  slot = Mix_LoadWAV(SoundPaths[index]);
  if (!slot) {
    SDL_Log("Failed to load '%s': %s", SoundPaths[index], Mix_GetError());
  }

  return slot;
}

inline void play(SoundID ID) {
  if (Mix_Chunk *chunk = get(ID))
    Mix_PlayChannel(-1, chunk, 0);
}

inline void playByChannel(SoundID ID, int channel = -1) {
  if (Mix_Chunk *chunk = get(ID))
    Mix_PlayChannel(channel, chunk, 0);
}

// ---------------- Music ----------------

inline Mix_Music *getMusic(MusicID ID) {
  const size_t index = static_cast<size_t>(ID);
  assert(index < static_cast<size_t>(MusicID::Count));
  Mix_Music *&slot = musicCache[index];

  if (slot) {
    return slot;
  }

  slot = Mix_LoadMUS(MusicPaths[index]);
  if (!slot) {
    SDL_Log("Failed to load '%s': %s", MusicPaths[index], Mix_GetError());
  }

  return slot;
}

// Check if SDL_mixer is playing music.
// Can return true when music is paused.
// Use isMusicPaused() if you need to tell playing from paused.
inline bool isMusicPlaying() {
  return Mix_PlayingMusic() != 0;
}
// Check if SDL_mixer paused music.
inline bool isMusicPaused() {
  return Mix_PausedMusic() != 0;
}
// Check if there's music playing and not paused.
inline bool isMusicAudible() {
  return Mix_PlayingMusic() && !Mix_PausedMusic();
}

// loops: -1 = loop forever (typical for background music), 0 = play once
inline void playMusic(MusicID ID, int loops = -1) {
  Mix_Music *music = getMusic(ID);
  if (!music)
    return;

  if (Mix_PlayMusic(music, loops) == -1) {
    SDL_Log("Failed to play music: %s", Mix_GetError());
    return;
  }
  currentMusic = ID;
}

// fadeMs = 0 for instant stop, or e.g. 1000 for a 1-second fade out
inline void stopMusic(int fadeMs = 0) {
  if (fadeMs > 0) {
    Mix_FadeOutMusic(fadeMs);
  } else {
    Mix_HaltMusic();
  }
}

inline void pauseMusic() {
  Mix_PauseMusic();
}

inline void resumeMusic() {
  Mix_ResumeMusic();
}

// volume: 0-128, matches SDL_mixer's native range (not 0-100)
inline void setMusicVolume(int volume) {
  Mix_VolumeMusic(volume);
}
}; // namespace SoundManager
