#include "game.cpp"

// Entry point: initializes the game, runs its loop, then shuts it down.
int main(int argc, char *argv[]) {
  if (!Game::init()) {
    return 1;
  }
  while (Game::loop()) {
  }
  Game::kill();
  return 0;
}
