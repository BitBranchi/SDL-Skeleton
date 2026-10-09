#pragma once

#include "input.cpp"
#include "resource_manager.cpp"
#include "texture.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_rect.h>
#include <SDL_render.h>
#include <SDL_ttf.h>
#include <SDL_video.h>
#include <iostream>

namespace Game {
std::string TITLE = "SDL2";
SDL_Window *WINDOW = NULL;
SDL_Renderer *RENDERER = NULL;
SDL_Event EVENT;
const int SCREEN_DIMENSION[2] = {1000, 600};
float PERFORMANCE_FREQUENCY = SDL_GetPerformanceFrequency();
Uint64 last_tick = SDL_GetPerformanceCounter();

bool init() {
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    std::cout << "Failed to init SDL2 : " << SDL_GetError();
    return false;
  }

  // -------- Init WIndow and Renderer --------
  WINDOW = SDL_CreateWindow(TITLE.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_DIMENSION[0], SCREEN_DIMENSION[1], SDL_WINDOW_SHOWN);
  if (!WINDOW) {
    std::cout << "Failed creating window : " << SDL_GetError();
    return false;
  }
  RENDERER = SDL_CreateRenderer(
      WINDOW, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!RENDERER) {
    std::cout << "Failed creating renderer : " << SDL_GetError();
    return false;
  }
  ResourceManager::tRenderer = RENDERER;
  // -------- Init other SDL2 extensions --------
  // These included : SDL_Image, SDL_Mixer, Open Audio
  int FLAGS = IMG_INIT_PNG | IMG_INIT_JPG;
  int INITTED = IMG_Init(FLAGS);
  if ((INITTED & FLAGS) != FLAGS) {
    std::cout << "Failed to init SDL_Image : " << IMG_GetError();
    return false;
  }
  FLAGS = MIX_INIT_OGG;
  INITTED = Mix_Init(FLAGS);
  if ((FLAGS & INITTED) != FLAGS) {
    std::cout << "Failed to init SDL_Mixer : " << Mix_GetError();
    return false;
  }
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    std::cout << "Failed to open audio : " << Mix_GetError();
    return false;
  }
  Mix_Volume(-1, (50.0f / 100.0f) * 128.0f); // Temp

  if (TTF_Init() < 0) {
    std::cout << "Failed to init SDL_TTF : " << TTF_GetError();
    return false;
  }

  // -------- Init Component Contexts --------
  return true;
}

//----------------------------------------------------------------------------------------//
// MAIN LOOP
//----------------------------------------------------------------------------------------//
bool loop() {
  while (SDL_PollEvent(&EVENT)) {
    switch (EVENT.type) {
    case SDL_QUIT:
      return false;
    default:
      break;
    }
  }
  Uint64 current_tick = SDL_GetPerformanceCounter();
  float dt = (float)((current_tick - last_tick) / PERFORMANCE_FREQUENCY);
  last_tick = current_tick;

  Input::update();
  if (Input::isKeyPressed(SDL_SCANCODE_ESCAPE)) {
    return false;
  }

  SDL_Rect r{0, 0, 32, 32};

  SDL_RenderCopy(RENDERER, ResourceManager::get(Texture::NO_TEXTURE), NULL, &r);
  SDL_RenderPresent(RENDERER);

  Input::lateUpdate();
  return true;
}

//----------------------------------------------------------------------------------------//
// KILL
//----------------------------------------------------------------------------------------//
void kill() {
  ResourceManager::tRenderer = NULL;
  ResourceManager::kill();
  SDL_DestroyRenderer(RENDERER);
  SDL_DestroyWindow(WINDOW);
  Mix_CloseAudio();
  IMG_Quit();
  Mix_Quit();
  TTF_Quit();
  SDL_Quit();
}
} // namespace Game
