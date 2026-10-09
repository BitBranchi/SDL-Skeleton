#include "game.cpp"

int main(int argc, char *argv[]) {
  if (!Game::init()) {
    return 1;
  }
  while (Game::loop()) {
  }
  Game::kill();
  return 0;
}
