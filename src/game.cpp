#include "camera.h"
#include "input.h"
#include "resource_manager.h"
#include "sound_manager.h"
#include "ui/ImState.h"
#include "ui/core/UIDraw.h"
#include "util/vector2.hpp"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_render.h>
#include <SDL_ttf.h>
#include <SDL_video.h>
#include <iostream>
#include <string>

namespace Game {
std::string TITLE = "SDL2";
SDL_Window *WINDOW = nullptr;
SDL_Renderer *RENDERER = nullptr;
SDL_Event EVENT;
const int SCREEN_DIMENSION[2] = {1000, 600};
Camera CAMERA;
CameraView VIEW;
const float PERFORMANCE_FREQUENCY = SDL_GetPerformanceFrequency();
Uint64 last_tick = SDL_GetPerformanceCounter();

// Advances one frame. Returns false when the game should stop.
bool loop() {
  // Event Polling
  while (SDL_PollEvent(&EVENT)) {
    switch (EVENT.type) {
    case SDL_QUIT:
      return false;
    default:
      break;
    }
  }

  // Clear Display
  SDL_SetRenderDrawColor(RENDERER, 0, 0, 0, 255);
  SDL_RenderClear(RENDERER);

  // Game Update
  const Uint64 current_tick = SDL_GetPerformanceCounter();
  const float dt = static_cast<float>(current_tick - last_tick) / PERFORMANCE_FREQUENCY; // Delta Time
  last_tick = current_tick;
  Input::update();
  UI::beginFrame(UI::readMouse(), dt);

  // <-- Your update code here

  // Camera follows CAMERA.target; every world draw uses VIEW.
  CAMERA.update(dt);
  VIEW = CameraView(CAMERA);

  // Rendering
  SDL_RenderPresent(RENDERER);

  // End-frame update
  UI::endFrame();
  Input::lateUpdate();
  return true;
}

// Game::init initializes every component. Call it once before the loop starts.
// Returns false on failure.
bool init() {
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    std::cout << "Failed to init SDL2 : " << SDL_GetError();
    return false;
  }

  // -------- Init Window and Renderer --------
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
  // -------- Init other SDL2 extensions --------
  // SDL_Image, SDL_Mixer, and audio
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
  CAMERA.deadzone = Vector2f{0.0f, 0.0f};
  CAMERA.viewSize = Vector2f{static_cast<float>(SCREEN_DIMENSION[0]),
                             static_cast<float>(SCREEN_DIMENSION[1])};
  CAMERA.worldMin = Vector2f{0.0f, 0.0f};
  CAMERA.worldMax = Vector2f{2000.0f, 1200.0f};
  ResourceManager::init(RENDERER);
  UI::init(RENDERER);
  UI::Draw::initNineSlice();
  SoundManager::init();
  return true;
}

// Game::kill tears down components in reverse init order. Call it once on shutdown.
void kill() {
  SoundManager::kill();
  UI::kill();
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
