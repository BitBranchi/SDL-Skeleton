# Usage Guide

Task-oriented recipes for the skeleton. For signatures, invariants and internals see [DOCUMENT.md](DOCUMENT.md).

---

## 1. Build and run

```sh
# one-time configure (Release is the default)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# build
cmake --build build -j

# run from the output folder or the repo root (assets are relative)
build/Debug/SDL2-Skeleton.exe
```

Requirements: `SDL2_ROOT` set to the SDL2 dev root (e.g. `E:\SDL2`), with sibling `_image`, `_ttf`, `_mixer` directories. Release links SDL statically; Debug links DLLs and copies them next to the exe. `assets/` is copied POST_BUILD, so edits to assets need a rebuild (or copy them by hand while iterating).

---

## 2. The frame you must not break

Every frame needs this order. `Game::loop()` already does it; keep it if you restructure.

```cpp
Input::update();                          // 1. snapshot input
UI::beginFrame(UI::readMouse(), dt);      // 2. reset draw stack, store mouse + dt

// ... game logic ...

CAMERA.update(dt);                        // 3. smooth the camera toward its target
VIEW = CameraView(CAMERA);                // 4. snapshot the world->screen transform

// ... world drawing (drawSprite with VIEW) and UI drawing ...

SDL_RenderPresent(RENDERER);              // 5. present
UI::endFrame();                           // 6. asserts the transform stack is balanced
Input::lateUpdate();                      // 7. current -> previous for edge detection
```

* `dt` comes from `SDL_GetPerformanceCounter()`; use it for anything time-based.
* Draw **before** `SDL_RenderPresent()`.
* Update `VIEW` after gameplay and before any world draw, so every sprite uses the same transform.
* If `UI::endFrame()` trips its assert, a `Draw::push()` has no matching `pop()` (or a `Draw::Scope` outlived the frame).

---

## 3. Add an asset

Asset IDs are enums with a `Count` sentinel, paired with a path array that a `static_assert` size-checks.

### 3.1 Texture

1. Drop the file in `assets/` (UI art goes in `assets/ui/`).
2. Add the ID in `src/texture.h` **before** `Count`.
3. Add the matching path at the same position in `TexturePaths`.

```cpp
enum class TextureID : size_t {
  NoTexture,
  PanelBright,
  // ...
  IconHeart,   // new
  Count,
};

inline constexpr std::array TexturePaths = {
    "assets/no_texture.png",
    // ...
    UI_PATH "icon_heart.png", // IconHeart
};
```

Use it:

```cpp
UI::image(TextureID::IconHeart, {20, 20, 32, 32});
// or raw, for your own drawing:
SDL_Texture *tex = ResourceManager::get(TextureID::IconHeart);
```

`get()` loads on first call and caches; keep the pointer instead of calling it every frame.

### 3.2 Sound effect

1. Put the file in `assets/sound/`.
2. Add `SoundID::Name` before `Count` and the path in `SoundPaths`.

```cpp
enum class SoundID : size_t { SHUTTER, Hover, Count };
inline constexpr std::array SoundPaths = {
    SOUND_PATH "shutter.ogg",
    SOUND_PATH "hover.ogg", // Hover
};
```

```cpp
SoundManager::play(SoundID::Hover);                    // any free channel
SoundManager::playByChannel(SoundID::Hover, 0);         // fixed channel
SoundManager::playWithPos(SoundID::Hover, 90, 40);      // angle, distance
```

SDL_mixer mixes the audio; you do not need to load anything up front.

### 3.3 Music

Same pattern with `MusicID` / `MusicPaths` under `assets/music/`. Only one track can play at a time.

```cpp
SoundManager::playMusic(MusicID::SHUTTER);   // loops forever (default)
SoundManager::stopMusic(1000);               // 1 s fade out
SoundManager::pauseMusic();
SoundManager::resumeMusic();
SoundManager::setMusicVolume(64);            // 0..128
```

### 3.4 Font

1. Put the TTF in `assets/font/`.
2. Add `FontID::Name` and the path in `FontPaths` (`src/ui/core/font.h`).
3. Point the theme at it, or render text directly.

```cpp
UI::Theme t = UI::Themes::Dark;
t.font = FontID::Title;
t.fontSize = 48;
```

A `TTF_Font` is opened per `(FontID, size)` and cached, so different sizes are fine.

---

## 4. Lay out a screen

```cpp
UI::panel({20, 20, 400, 300});
UI::label(ID_TITLE, {20, 20, 400, 40}, "Settings", UI::Align::Center);
UI::image(TextureID::IconHeart, {40, 250, 32, 32});
UI::progressBar({90, 250, 300, 32}, health);   // fraction, clamped to 0..1
```

* `panel`/`image`/`progressBar` are stateless — no ID needed.
* `label` needs an ID only because it caches its rendered text under it. Empty strings draw nothing and allocate nothing.
* Long text: `UI::label(ID_BODY, {20, 70, 380, 200}, body, UI::Align::Left, true);` — the `true` wraps at the rect width and left-aligns the wrapped block.
* `progressBar` does not animate; feed it a tweened float if you want smooth changes.

---

## 5. Interactive widgets

Give every interactive widget a unique `int` ID. An enum per screen is the easiest way to stay unique:

```cpp
enum Widget : int {
  ID_PLAY = 1,
  ID_MUTE,
  ID_VOLUME,
  ID_TITLE, // labels share the same map, keep them in the same enum
};
```

```cpp
if (UI::button(ID_PLAY, {20, 20, 200, 40}, "Play")) {
  startGame();
}

bool mute = false;            // your state, not the widget's
if (UI::toggle(ID_MUTE, {20, 70, 40, 40}, mute, "Mute")) {
  SoundManager::play(mute ? SoundID::Hover : SoundID::SHUTTER);
}

float volume = 0.5f;
if (UI::slider(ID_VOLUME, {20, 130, 200, 40}, volume, 0.0f, 1.0f, 0.05f)) {
  SoundManager::setMusicVolume(static_cast<int>(volume * 128.0f));
}
```

Notes:

* `button` returns true only on the frame of a completed click (press **and** release inside the same widget).
* `toggle` returns true on the frame `value` flips; the box is `r.h` square and the text sits to its right.
* `slider` keeps tracking the drag after the cursor leaves the rect, until release. `step > 0` snaps the value; pass `0.0f` for continuous.
* Keep widget state (the `bool`, the `float`) in your own game state — widgets only remember hover/press/animations.
* `enabled = false` draws at `theme().disabledAlpha` and ignores the mouse.

---

## 6. React to hover/press without a return value

The query helpers read the state the widget stored this frame:

```cpp
if (UI::button(ID_PLAY, r, "Play")) { startGame(); }

if (UI::hoverEntered(ID_PLAY))
  SoundManager::play(SoundID::Hover);
if (UI::isDown(ID_PLAY))
  drawPressedHighlight = true;
```

Order matters: query **after** the widget call, and the ID must have been used this frame.

---

## 7. Themes

```cpp
if (UI::toggle(ID_THEME, {340, 20, 40, 40}, darkMode)) {
  UI::setTheme(darkMode ? UI::Themes::Dark : UI::Themes::Bright);
}
```

`setTheme` only stores a pointer, so the theme must be a global/`constexpr` object:

```cpp
inline const UI::Theme Vintage = [] {
  UI::Theme t = UI::Themes::Bright;
  t.panel  = {TextureID::PanelBright, 8};
  t.button = {TextureID::ButtonBright, 8};
  t.squish = 4.0f;
  t.pressOffset = 3.0f;
  t.animSeconds = 0.08f;
  t.fontSize = 20;
  t.textColor = {40, 30, 20, 255};
  return t;
}();
```

Never pass a local `Theme` to `setTheme` — it dangles immediately. Theme fields also feed the text cache, so changing `font`/`fontSize`/`textColor` re-renders labels automatically on the next frame.

---

## 8. Animate

### 8.1 Ease a value

```cpp
UI::Tween healthBar(0.4f, UI::Ease::OutCubic);   // duration, easing

healthBar.setTarget(health > 0.0f);
healthBar.update(dt);
UI::progressBar({20, 20, 200, 24}, healthBar.value());
```

`setTarget(bool)` only handles 0↔1; for arbitrary values use `lerp` with `value()`:

```cpp
float x = UI::lerp(closedX, openX, panelTween.value());
```

`jumpTo(bool)` skips the animation entirely.

### 8.2 Slide or fade a group of widgets

`Draw::Scope` pushes a transform for the rest of the scope and pops it automatically:

```cpp
UI::Tween slide(0.2f, UI::Ease::OutCubic);
slide.setTarget(open);
slide.update(dt);

{
  UI::Draw::Scope move(UI::Draw::Transform{-300.0f * (1.0f - slide.value()), 0.0f, 1.0f});
  UI::panel({0, 0, 300, 600});
  if (UI::button(ID_CLOSE, {20, 20, 260, 40}, "Close")) open = false;
}
```

Transforms compose (offsets add, alphas multiply) and hit-testing follows them, so widgets inside a sliding panel are clickable where they appear. Alpha below ~0.01 disables interaction.

For a fade, animate the third field:

```cpp
UI::Draw::Scope fade(UI::Draw::Transform{0.0f, 0.0f, slide.value()});
```

Prefer one shared `Tween` over ad-hoc `sin()` timing: the same object handles open and close by retracing the curve.

### 8.3 Custom widget animation

Everything animating uses `Tween`; `hoverT`/`pressT` live in the widget state and are driven by `theme().animSeconds`. To animate something not covered by a widget, keep a `Tween` next to the state it feeds and call `update(dt)` before drawing.

---

## 9. Text beyond `label`

`label` always uses `TextMode::Solid` (crisp, pixel-art friendly). For anti-aliased or manually placed text, own a `TextTexture`:

```cpp
UI::TextTexture cache;   // member of your screen; move-only

cache.set("SCORE 1200", FontID::Main, 32, SDL_Color{255, 220, 80, 255},
          0, UI::TextMode::Blended);

UI::Draw::text(cache, 20.0f, 20.0f);                 // top-left at (x, y)
UI::Draw::textCentered(cache, SDL_FRect{0, 0, 1000, 60});
```

Call `set()` only when the string/font/size/color changes — it allocates a texture. `wrapWidth > 0` wraps at that pixel width; `0` wraps only on `'\n'`.

`TextTexture` is move-only (it owns the texture) and safe to destroy after `UI::kill()`.

---

## 10. Input recipes

```cpp
// Movement
Vector2f dir{0.0f, 0.0f};
if (Input::isKeyHeld(SDL_SCANCODE_A)) dir.x -= 1.0f;
if (Input::isKeyHeld(SDL_SCANCODE_D)) dir.x += 1.0f;
if (Input::isKeyHeld(SDL_SCANCODE_W)) dir.y -= 1.0f;
if (Input::isKeyHeld(SDL_SCANCODE_S)) dir.y += 1.0f;
if (dir != Vector2f::zero()) dir.normalize();
playerPos += dir * 200.0f * dt;

// Edge-triggered actions
if (Input::isKeyPressed(SDL_SCANCODE_SPACE)) jump();
if (Input::isKeyReleased(SDL_SCANCODE_TAB)) nextFocus();

// Mouse
if (Input::isMouseButtonPressed(SDL_BUTTON_LEFT)) {
  Vector2i p = Input::getMousePos();
  spawnAt(p.toSDLFPoint());
}
```

`isKeyPressed` is true on exactly one frame, which requires `Input::lateUpdate()` at the end of the frame. `SDL_SCANCODE_*` (physical keys) is used everywhere; `SDL_GetKeyFromScancode` converts if you need a key symbol.

---

## 11. Vectors and randomness

```cpp
Vector2f a{3.0f, 4.0f};
Vector2f b = a.normalized();              // {0.6, 0.8}
float d = a.distanceTo({0.0f, 0.0f});     // 5
Vector2f c = a.rotated(1.5707963f);       // 90 degrees
Vector2f mid = Vector2f::lerp(a, b, 0.5f);
SDL_FPoint fp = a.toSDLFPoint();
```

```cpp
#include "util/randomizer.h"   // not included by game.cpp yet

Random rng(1234);                  // seed it for reproducible runs
int damage   = rng.range(5, 10);   // inclusive
float speed  = rng.range(0.8f, 1.2f); // half-open
bool crit    = rng.chance(0.1);
Vector2f dir = rng.unitVector();
Vector2f pos = rng.inCircle(50.0f);
rng.shuffle(enemies.begin(), enemies.end());
size_t tier  = rng.weighted(weights);
```

Use one `Random` (a global `rng` already exists) and `split()` when you need an independent stream. Seed explicitly in tests or replays.

---

## 12. Camera and world drawing

`Game::CAMERA` is the follow camera; `Game::VIEW` is its per-frame transform. Configure bounds in `Game::init()`, move `target` during gameplay, and let `Game::loop()` update/snapshot it.

```cpp
// In Game::init():
CAMERA.viewSize = {static_cast<float>(SCREEN_DIMENSION[0]), static_cast<float>(SCREEN_DIMENSION[1])};
CAMERA.worldMin = {0.0f, 0.0f};
CAMERA.worldMax = {2000.0f, 1200.0f}; // equal min/max disables clamping
CAMERA.deadzone = {80.0f, 60.0f};     // half-size of the free-move box
CAMERA.follow = 6.0f;                 // higher = snappier

// In the game-logic block:
CAMERA.target = player.position;

// Drawing a world sprite (VIEW is already updated for this frame):
SDL_FRect world{player.position.x, player.position.y, 32.0f, 32.0f};
drawSprite(RENDERER, tex, nullptr, world, VIEW);
```

* `CAMERA.pos` is the camera center in world space; `VIEW.origin` is the top-left world point. They are different.
* Mouse → world: `Vector2f world = VIEW.toWorld(Vector2f(Input::getMousePos()));`
* `drawSprite` culls, transforms, snaps to whole pixels and draws. Pass `false` as the last argument for sub-pixel motion.
* UI drawn with `UI::Draw`/widgets is screen space and ignores the camera. Do not mix the two coordinate spaces.

---

## 13. Screen changes

* Keep per-screen widget state minimal — widgets only cache hover/press animations and text. If you want a fresh screen to animate from zero, call `UI::clearState()`; it drops every cached text texture too, so it is cheap only on transitions.
* `clearState()` is safe while the renderer exists (i.e. not after `Game::kill()`).
* Theme changes alone do not clear widget state, but they invalidate cached text via the theme fields.

---

## 14. Common mistakes

| Symptom | Cause |
| --- | --- |
| Button never clicks | `Input::lateUpdate()` missing, or the widget state ID is reused by another widget |
| Animations frozen | `UI::beginFrame()` not called, so `frame.dt` stays 0 |
| Widget not clickable inside a panel | Hit area is the base rect **plus the current transform** — make sure the `Draw::Scope` is active when the widget is called |
| Stale click when the mouse leaves and returns | `PressState::down` is only cleared on release or when the button is no longer held |
| Text updates every frame (slow) | Calling `TextTexture::set()` per frame instead of only when the content changes; `label` already caches |
| Text renders blurry / stretched | Use `UI::Draw::text` (position-only snapping); do not scale a text texture into a different-sized rect |
| Assert: unbalanced `Draw::push / pop` | A `Scope` created outside the `beginFrame`/`endFrame` window, or a manual `push()` without `pop()` |
| Everything disappears after `setTheme` | Theme was a local variable; it must be a global/`constexpr` object |
| Hover flicker on a squishing button | Do not hit-test the animated rect — use the base rect (widgets already do) |
| Camera does not move | `CAMERA.target` was never set, or `CAMERA.update(dt)` / `VIEW = CameraView(CAMERA)` is missing from the frame |
| Sprites or UI appear offset/scroll with the world | Mixing coordinate spaces — sprites need `VIEW`/`drawSprite`, UI needs `UI::Draw` |
| New code "not found" by the compiler | Only `main.cpp` is compiled; new `.cpp` files need `SOURCE_FILES` and a removed `#include` |
| Asset not found at runtime | Working directory is not `build/<Config>` or the repo root, or the asset was added after the last POST_BUILD copy |

---

## 15. Where to put new code

* Gameplay state belongs in `Game` (or a `Game::State`-style struct) inside `game.cpp`, so it does not pollute the UI toolkit.
* Reusable helpers go in headers under `src/` (or `src/util/`), written as `inline` definitions.
* Never add a `.cpp` unless you also change `SOURCE_FILES`; the build compiles `src/main.cpp` only.
* Keep new drawing inside the `beginFrame`/`endFrame` window and before `SDL_RenderPresent()`.
