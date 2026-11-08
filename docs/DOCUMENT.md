# SDL2-Skeleton — Documentation & Reference

Hand-written SDL2 skeleton in C++17: one game loop, a follow camera, an immediate-mode UI toolkit, texture/audio caches and a small math/util layer. Everything is header-only apart from `src/main.cpp` (which pulls in `src/game.cpp`).

---

## 1. Build & toolchain

| Item | Value |
| --- | --- |
| Standard | C++17 (`CMAKE_CXX_EXTENSIONS OFF`) |
| Output | `build/<Config>/SDL2-Skeleton.exe` (`RUNTIME_OUTPUT_DIRECTORY`) |
| Default config | `Release` (static SDL, `-static`) — use `Debug` for iteration (shared DLLs) |
| SDL2 root | `SDL2_ROOT` env var / CMake cache var (currently `E:\SDL2`) |
| Sibling deps | `${SDL2_ROOT}_image`, `${SDL2_ROOT}_ttf`, `${SDL2_ROOT}_mixer`, each with `x86_64-w64-mingw32/` |
| Assets | `${CMAKE_SOURCE_DIR}/assets` (override with `-DASSETS_SOURCE_DIR=...`), copied next to the exe POST_BUILD |
| IntelliSense | `.clangd` → `C:/msys64/ucrt64/bin/g++.exe`; `compile_commands.json` is exported |

```sh
cmake -S . -B build
cmake --build build -j
```

Asset paths in code are relative (`assets/...`), so run from `build/<Config>` or the repo root.

### Single translation unit

`CMakeLists.txt` compiles only `src/main.cpp`. `main.cpp` does `#include "game.cpp"`, so `game.cpp` and every header it reaches is part of one TU.

* Headers define things (`inline` variables/functions) instead of declaring them — there are no `.cpp` implementations and no link step for new code.
* A new `.cpp` is **not** built unless added to `SOURCE_FILES` and its `#include` removed from `game.cpp`/`main.cpp`.
* Consequences: full-rebuild-like compile times, no ODR issues between modules, order of `#include` matters only for what a header needs.

---

## 2. Source layout

| Path | Role |
| --- | --- |
| `src/main.cpp` | Entry point: `init` → `while (loop())` → `kill` |
| `src/game.cpp` | SDL setup, window/renderer ownership, frame loop, demo screen, shutdown |
| `src/camera.h` | `Camera` follow/zoom/bounds state, `CameraView` world→screen transform + culling, `drawSprite` |
| `src/input.h` | Keyboard/mouse snapshot with per-frame edge detection |
| `src/texture.h` | `TextureID` enum + `TexturePaths` table (+ `static_assert` size check) |
| `src/resource_manager.h` | Lazy `SDL_Texture` cache |
| `src/sound.h` | `SoundID` / `MusicID` enums + path tables |
| `src/sound_manager.h` | Lazy `Mix_Chunk` / `Mix_Music` cache, playback, panning/position, music transport |
| `src/UI.h` | Umbrella header for the immediate-mode UI |
| `src/ui/core/` | Shared UI core: theme, tweens, renderer context, draw backend, fonts, text textures, hit-testing, types |
| `src/ui/ImState.h` | Per-frame state, per-widget state map, hover/press animation, text caching, query helpers |
| `src/ui/ImWidgets.h` | Widgets: `panel`, `image`, `label`, `button`, `toggle`, `slider`, `progressBar` |
| `src/util/vector2.hpp` | `Vector2<T>` math type with SDL interop |
| `src/util/randomizer.h` | Deterministic `Random` (xoshiro256**) utility — not yet included by the game |

Dependency direction: `UI.h` → `ui/ImWidgets.h` → `ui/ImState.h` → `ui/core/UICore.h` → `texture.h` / `resource_manager.h` / `input.h`.

---

## 3. Program lifecycle

```
main()
├── Game::init()            // false on failure → exit code 1
├── while (Game::loop()) {} // false on SDL_QUIT / ESC → exit loop
└── Game::kill()
```

### `Game::init()` — strict order

1. `SDL_Init(SDL_INIT_EVERYTHING)`
2. `SDL_CreateWindow` (`SDL_WINDOW_SHOWN`, centered, 1000×600) and `SDL_CreateRenderer` (`ACCELERATED | PRESENTVSYNC`)
3. `IMG_Init(PNG | JPG)`
4. `Mix_Init(OGG)`, `Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048)` (master volume 50 %)
5. `TTF_Init()`
6. Configure `CAMERA` (deadzone, `viewSize`, world bounds)
7. `ResourceManager::init(renderer)`
8. `UI::init(renderer)`, `UI::Draw::initNineSlice()`
9. `SoundManager::init()`

Each step prints to `std::cout` and returns `false` on failure; `init` is call-once.

### `Game::loop()` — per-frame order

```
poll events (SDL_QUIT → return false)
clear to black
dt = (now - last_tick) / SDL_GetPerformanceFrequency()
Input::update()                     // snapshot keyboard + mouse
UI::beginFrame(UI::readMouse(), dt) // store mouse/dt, reset draw stack
[game logic / widgets]
CAMERA.update(dt)                   // smooth toward CAMERA.target
VIEW = CameraView(CAMERA)           // snapshot transform for this frame
SDL_RenderPresent()
UI::endFrame()                      // assert transform stack balanced
Input::lateUpdate()                 // current → previous for next frame
```

Draw calls must happen **before** `SDL_RenderPresent()`; anything drawn after it lands in the next frame. Camera updates run after gameplay has moved things and before any world drawing, so every draw sees the same `VIEW`.

### `Game::kill()` — reverse order

`SoundManager::kill()` (frees chunks/music) → `UI::kill()` (closes fonts, nulls the renderer) → `ResourceManager::kill()` (destroys textures) → destroy renderer → destroy window → `Mix_CloseAudio` → `IMG_Quit` → `Mix_Quit` → `TTF_Quit` → `SDL_Quit`.

`UI::kill()` must run before `SDL_DestroyRenderer()`: cached `TextTexture`s check `UI::renderer` in their destructor and skip `SDL_DestroyTexture` once it is null (the renderer already freed them).

### `Game` globals

| Symbol | Type | Notes |
| --- | --- | --- |
| `Game::TITLE` | `std::string` | Window title, `"SDL2"` |
| `Game::WINDOW` / `Game::RENDERER` | `SDL_Window*` / `SDL_Renderer*` | Owned by the game; passed to the other modules |
| `Game::EVENT` | `SDL_Event` | Polled in `loop()` |
| `Game::SCREEN_DIMENSION` | `const int[2]` | `{1000, 600}` |
| `Game::PERFORMANCE_FREQUENCY`, `last_tick` | `float`, `Uint64` | Delta-time source |
| `Game::CAMERA` / `Game::VIEW` | `Camera` / `CameraView` | Follow state and this frame's world→screen snapshot |

---

## 4. Input — `namespace Input` (`input.h`)

State is captured once per frame, and "previous" state is snapshotted at the very end of the frame, which is what makes edge detection work.

| Function | Returns | Notes |
| --- | --- | --- |
| `update()` | — | `SDL_GetKeyboardState` + `SDL_GetMouseState`; call once per frame before any query |
| `lateUpdate()` | — | Copies current → previous; call last |
| `isKeyHeld(SDL_Scancode)` | `bool` | Held now |
| `isKeyPressed(SDL_Scancode)` | `bool` | Up→down this frame |
| `isKeyReleased(SDL_Scancode)` | `bool` | Down→up this frame |
| `getMousePos()` | `Vector2i` | Position from the last `update()` |
| `isMouseButtonHeld(Uint8)` | `bool` | `SDL_BUTTON_LEFT`/`MIDDLE`/`RIGHT`/`X1`/`X2` |
| `isMouseButtonPressed(Uint8)` | `bool` | Down this frame |
| `isMouseButtonReleased(Uint8)` | `bool` | Up this frame |

Implementation detail: keyboard edges rely on `prevKeyState`, an `unordered_map` populated lazily by the first query for a scancode. A key queried for the first time reports `prev == false`, so if it is already down it reads as *pressed* that frame.

---

## 5. Textures — `texture.h` + `namespace ResourceManager`

### Asset registration

`TextureID` (enum class, index into `TexturePaths`):

| ID | Path |
| --- | --- |
| `NoTexture` | `assets/no_texture.png` (fallback) |
| `PanelBright` | `assets/ui/panel_bright.png` |
| `PanelDark` | `assets/ui/panel_dark.png` |
| `ButtonBright` | `assets/ui/button_bright.png` |
| `ButtonDark` | `assets/ui/button_dark.png` |
| `KnobBright` | `assets/ui/knob_bright.png` |
| `KnobDark` | `assets/ui/knob_dark.png` |
| `FillBright` | `assets/ui/fill_bright.png` |
| `FillDark` | `assets/ui/fill_dark.png` |
| `CheckBright` | `assets/ui/check_bright.png` |
| `CheckDark` | `assets/ui/check_dark.png` |
| `Count` | sentinel, not a texture |

`static_assert(TexturePaths.size() == static_cast<size_t>(TextureID::Count))` forces one path per ID.

### API

| Function | Description |
| --- | --- |
| `init(SDL_Renderer*)` | Stores the renderer and sizes the cache to `TextureID::Count`; call before any `get` |
| `get(TextureID)` | Loads on first request via `IMG_LoadTexture`, then cached. On failure logs and falls back to `NoTexture` (which may also be null) |
| `ensurePanelRect(TextureID = PanelBright)` | Legacy 3×3 source-rect split into `panelRect[9]` using `PANEL_BORDER = 48`; unused by the current nine-slice renderer |
| `kill()` | Destroys all cached textures and clears state |

`get()` asserts `ID < Count`; fetch a pointer once and keep it rather than calling per frame.

---

## 6. Audio — `sound.h` + `namespace SoundManager`

### Asset registration

`SoundID`: `SHUTTER` → `assets/sound/shutter.ogg`
`MusicID`: `SHUTTER` → `assets/music/shutter.ogg`

Both enums end with a `Count` sentinel, and both path arrays are size-checked with `static_assert`.

### Effects (`Mix_Chunk`)

| Function | Description |
| --- | --- |
| `init()` | Sizes both caches; call before first playback |
| `kill()` | Frees all chunks and music (does not reset `currentMusic`) |
| `get(SoundID)` | Lazy load with `Mix_LoadWAV` (SDL_mixer also accepts OGG here); `nullptr` on failure |
| `play(SoundID)` | One-shot on any free channel |
| `playByChannel(SoundID, int channel = -1)` | One-shot on a chosen channel, `-1` = any |
| `playWithPos(SoundID, Sint16 angle, Uint8 distance, int channel = -1)` | Plays then applies positional audio; no-op if no channel was assigned |
| `setChannelPanning(channel, Uint8 left = 255, Uint8 right = 255)` | 0 = silent, 255 = loudest; ignores `channel == -1` |
| `setChannelDistance(channel, Uint8 distance = 0)` | 0 = at listener, 255 = furthest |
| `setChannelPosition(channel, Sint16 angle = 90, Uint8 distance = 0)` | `0` north, `90` east, … |

### Music (`Mix_Music`)

Music is streamed, and SDL_mixer can only play **one** track at a time.

| Function | Description |
| --- | --- |
| `getMusic(MusicID)` | Lazy `Mix_LoadMUS`, `nullptr` on failure |
| `playMusic(MusicID, int loops = -1)` | `-1` loops forever, `0` once; records `currentMusic` on success |
| `stopMusic(int fadeMs = 0)` | `Mix_FadeOutMusic` when `fadeMs > 0`, else `Mix_HaltMusic` |
| `pauseMusic()` / `resumeMusic()` | Transport controls |
| `setMusicVolume(int volume)` | SDL_mixer native range `0–128` |
| `isMusicPlaying()` | True while playing **or paused** |
| `isMusicPaused()` | True while paused |
| `isMusicAudible()` | Playing **and** not paused |

---

## 7. UI core — `src/ui/core/`

`UICore.h` is the umbrella header: `Theme.h`, `Tween.h`, `UIContext.h`, `UIDraw.h`, `UIFont.h`, `UIInput.h`, `UIText.h`, `UITypes.h`.

### 7.1 Renderer context (`UIContext.h`)

| Symbol | Description |
| --- | --- |
| `inline SDL_Renderer *renderer` | Shared renderer used by text and draw code |
| `init(SDL_Renderer*)` | Must run after `SDL_CreateRenderer`, `TTF_Init`, `ResourceManager::init` |
| `kill()` | Calls `Font::kill()`, then nulls `renderer`; must run before `SDL_DestroyRenderer` and `TTF_Quit` |
| `ready()` | `renderer != nullptr` |

### 7.2 Types (`UITypes.h`)

* `toF(const SDL_Rect&) -> SDL_FRect` — converts.
* `snap(const SDL_FRect&) -> SDL_Rect` — rounds the **edges** (left/top and right/bottom) instead of the size, so adjacent nine-slice pieces share an edge and never show a 1 px seam.

### 7.3 Theme (`Theme.h`)

```cpp
struct NineSlice { TextureID texture = TextureID::NoTexture; int border = uiBorder; }; // uiBorder == 8

struct Theme {
  NineSlice panel, button, buttonHover, track, fill, handle, toggleOff, toggleOn;
  FontID font = FontID::Main;
  int fontSize = 24;
  SDL_Color textColor{255,255,255,255};
  float squish = 6.0f;         // hover growth in px (wider / shorter)
  float pressOffset = 2.0f;    // px a pressed button drops
  float animSeconds = 0.15f;   // hover/press animation length
  float disabledAlpha = 0.5f;  // alpha of disabled widgets
};
```

| Symbol | Description |
| --- | --- |
| `Themes::Dark`, `Themes::Bright` | `constexpr Theme` presets built by `makeDark()` / `makeBright()` |
| `setTheme(const Theme&)` | Points `active` at the theme — the object must outlive its use (global/constexpr, never a local) |
| `theme()` | `const Theme&` currently active; defaults to `Themes::Dark` |

`buttonHover` doubles as the pressed look. The presets reuse panel/button/knob/fill/check textures, so no extra art is required.

### 7.4 Tweens (`Tween.h`)

```cpp
enum class Ease { Linear, InQuad, OutQuad, InOutQuad, OutCubic, Overshoot };
float ease(Ease, float t);                       // t clamped to 0..1
float lerp(float a, float b, float t);

struct Tween {
  float progress = 0, target = 0, duration = 0.2f;
  Ease easing = Ease::OutQuad;
  Tween(); Tween(float seconds, Ease e = Ease::OutQuad);
  void setTarget(bool on);
  void jumpTo(bool on);      // snap to an end, no animation
  void update(float dt);     // moves progress at 1/duration per second
  float value() const;       // ease(easing, progress)
  bool done() const;
};
```

Easing is applied on read, so reversing the target retraces the same curve. `Overshoot` peaks near 1.25 and settles at 1.

### 7.5 Fonts (`font.h`, `UIFont.h`)

`FontID`: `Main` → `assets/font/font.ttf`, plus `Count` sentinel; `FontPaths` is size-checked.

`TTF_Font` is tied to one size, so the cache is keyed by `(FontID << 32) | size`.

| Function | Description |
| --- | --- |
| `Font::get(FontID, int size)` | Opens on first request; logs and returns `nullptr` on failure (failures are not cached) |
| `Font::kill()` | Closes every cached font; call before `TTF_Quit` |

`makeKey(FontID, int)` is exposed for the cache key. Lookups only happen when text is (re)rendered, never per frame.

### 7.6 Text textures (`UIText.h`)

`enum class TextMode { Solid, Blended }` — `Solid` is aliased/crisp (pixel art), `Blended` is anti-aliased.

`class TextTexture` owns one `SDL_Texture`, move-only:

| Member | Description |
| --- | --- |
| `set(message, fontId, size, color, wrapWidth = 0, mode = Solid)` | Renders once via `TTF_RenderText_*_Wrapped`; `wrapWidth == 0` wraps only on `'\n'`; empty text is valid and draws nothing; returns `false` on failure (texture left empty) |
| `destroy()` | Frees the texture; skips freeing after `UI::kill()` |
| `texture()`, `width()`, `height()`, `valid()` | Accessors |

Call `set()` only when the string, font, size, color, wrap or mode changes — never per frame.

### 7.7 Hit testing (`UIInput.h`)

```cpp
struct MouseState { int x = 0, y = 0; bool down, pressed, released; };
MouseState readMouse(Uint8 button = SDL_BUTTON_LEFT);   // call after Input::update()
bool contains(const SDL_FRect&, int x, int y);         // half-open: [x, x+w) × [y, y+h)

struct PressState { bool hover, down, clicked; };
void updatePress(PressState&, const SDL_FRect&, const MouseState&); // + SDL_Rect overload
```

`updatePress` semantics:

* `hover` — cursor inside the base rect.
* `down` — a press started inside and the button is still held.
* `clicked` — true for exactly one frame: released while still inside.
* Releasing outside the window while `down` (no `released` event seen) clears `down`.

Hit-testing always uses the **base/layout** rect, never the animated draw rect, otherwise squish animations would feed back into hover and flicker. Overlapping widgets are not resolved: every widget under the cursor reacts.

### 7.8 Drawing backend (`UIDraw.h`, `namespace UI::Draw`)

A transform stack of `{x, y, alpha}` (no scale; widgets compute their own draw rect) plus immediate-mode primitives.

| Function / symbol | Description |
| --- | --- |
| `initNineSlice()` | Builds the shared 54-index buffer for a 4×4 vertex grid (18 triangles); call once at startup |
| `struct Transform { float x, y, alpha; }` | Offset + alpha applied to everything drawn while pushed |
| `stack`, `current()` | Stack (starts with one identity entry) and its top |
| `push(t)`, `pop()` | Compose: offsets add, alphas multiply; `pop()` asserts |
| `struct Scope` | RAII `push`/`pop`; non-copyable |
| `beginFrame()` | `stack.resize(1)` so a missing `pop` cannot leak into the next frame |
| `copy(tex, src, dst, exactSize = false)` | Applies the transform, snaps (edges, or position only when `exactSize`), applies alpha via `SDL_SetTextureAlphaMod`, then restores alpha 255 |
| `texture(TextureID, dstF)` | Whole texture stretched into `dst` |
| `nineSlice(const NineSlice&, dstF)` | GPU nine-slice via `SDL_RenderGeometry`; source border clamped to half the texture, destination border to half `dst` (tiny panels shrink corners); vertices are snapped to whole pixels |
| `text(texture, x, y)` | Text at native size, top-left at `(x, y)`, position-only snapping so glyphs never stretch |
| `textCentered(texture, boxF)` | Centers a `TextTexture` in a rect |

`copy`/`nineSlice` return early when the resolved alpha is 0 (`< 0.01` in draw terms is not used; the check is `alpha == 0` after quantisation).

---

## 8. Immediate-mode UI — `ImState.h`, `ImWidgets.h`

### 8.1 Frame state

```cpp
struct FrameData { MouseState mouse; float dt = 0.0f; };
beginFrame(const MouseState&, float dt); // stores both and calls Draw::beginFrame()
endFrame();                              // asserts Draw::stack.size() == 1
```

`endFrame()` is a balance check: a forgotten `Draw::pop()` (or a `Scope` that outlived the frame) trips the assert.

### 8.2 Per-widget state (`UI::Im`)

```cpp
struct WidgetState {
  PressState press;
  bool hoverEntered;   // true only on the frame the cursor arrived
  Tween hoverT, pressT;
  TextTexture text;
  TextKey key;
};
inline std::unordered_map<int, WidgetState> states;
```

| Function | Description |
| --- | --- |
| `ensureText(WidgetState&, std::string_view, int wrap)` | Returns the cached text texture; re-renders only when text, theme font, size, color or wrap changed. `valid` is set even on failure so a bad font logs once |
| `interact(int id, const SDL_Rect& base, bool enabled)` | Hit-tests using the base rect moved by the current `Draw` transform (so a widget inside a sliding panel is clickable where it appears), resets state when disabled or nearly transparent (`alpha <= 0.01`), drives `hoverT` (`Overshoot`) and `pressT` (`OutQuad`) with `theme().animSeconds` |

### 8.3 Queries

Valid after the widget with that ID has been called this frame.

| Function | Description |
| --- | --- |
| `isHovered(int id)` | Cursor over the widget |
| `isDown(int id)` | Widget currently pressed |
| `hoverEntered(int id)` | One-frame hover-enter edge (use for hover sounds) |
| `clearState()` | Clears every widget's state (animations + cached text); safe while the renderer exists |

### 8.4 Widgets

`enum class Align { Left, Center, Right }`.

| Widget | Signature | Behavior |
| --- | --- | --- |
| `panel` | `void panel(const SDL_Rect&)` | `theme().panel` nine-sliced |
| `image` | `void image(TextureID, const SDL_Rect&)` | Texture stretched into the rect |
| `progressBar` | `void progressBar(const SDL_Rect&, float fraction)` | `fraction` clamped to 0..1, drawn as `track` + `fill`; animate it yourself with a `Tween` |
| `label` | `void label(int id, const SDL_Rect&, std::string_view, Align = Center, bool wrap = false)` | Text cached under `id`; `wrap` wraps at `r.w` and left-aligns within the block; empty text returns early |
| `button` | `bool button(int id, const SDL_Rect&, std::string_view, bool enabled = true)` | True on the click frame (press **and** release inside). Grows `theme().squish` px wider / shorter while hovered, drops `pressOffset` while pressed |
| `toggle` | `bool toggle(int id, const SDL_Rect&, bool& value, std::string_view text = {}, bool enabled = true)` | Square box of side `r.h`, whole rect clickable, optional text 8 px to the right; returns true on the frame `value` flips |
| `slider` | `bool slider(int id, const SDL_Rect&, float& value, float minV = 0.0f, float maxV = 1.0f, float step = 0.0f, bool enabled = true)` | Click/drag anywhere on the rect; drag keeps tracking outside the rect until release; `step > 0` snaps; handle width = `clamp(r.h, 1, max(1, r.w))`; returns true on frames where the value changed |

Widget IDs are plain `int`s you choose (an enum per screen works well). They must be **unique across every widget type**, because all widgets share one state map. As a rule of thumb, IDs of stateless widgets (`panel`, `image`, `progressBar`) are unnecessary; `label` needs one only for its text cache.

---

## 9. Utilities

### 9.1 `Vector2<T>` (`util/vector2.hpp`)

`static_assert` requires an arithmetic `T`. Aliases: `Vector2f`, `Vector2d`, `Vector2i`.

| Group | Members |
| --- | --- |
| Construction | `Vector2()` (zero), `Vector2(x, y)`, explicit converting constructor from `Vector2<U>` |
| SDL interop | `toSDLPoint()`, `toSDLFPoint()`, `fromSDLPoint()`, `fromSDLFPoint()` |
| Arithmetic | `+`, `-`, unary `-`, `*`/`/` by scalar, component-wise `*`/`/`, `+=`, `-=`, `*=`, `/=`; free `scalar * v` |
| Comparison | `==`, `!=` |
| Math | `dot`, `cross` (scalar z-component), `lengthSquared`, `length`, `distanceTo`, `distanceSquaredTo`, `normalized()` (zero-safe → `{0,0}`), `normalize()`, `angle()` (radians, `atan2`), `rotated(radians)`, `perpendicular()` (90° CCW), `static lerp(a, b, t)` |
| Constants | `zero()`, `one()` |
| Streaming | `operator<<` prints `(x, y)` |

Integer instantiations still expose float-ish helpers (`length`, `angle`, `rotated`) — results are cast back to `T`.

### 9.2 `Random` (`util/randomizer.h`)

Header-only, xoshiro256** seeded through SplitMix64, unbiased integer sampling, no `rand()`/`std::shuffle`/standard distributions. Deterministic for a given seed across compilers. A global instance `rng` exists; the header is currently not included anywhere in the game.

| Member | Returns | Notes |
| --- | --- | --- |
| `Random()` / `Random(uint64_t seed)` | — | Default seed from `std::random_device`, clock fallback |
| `seed(uint64_t)`, `seedValue()` | — / `uint64_t` | Reseed / read back |
| `next()` | `uint64_t` | Raw generator output |
| `split()` | `Random` | Independent child generator |
| `below(uint64_t n)` | `uint64_t` | Uniform in `[0, n)`; `0` for `n == 0` |
| `range(Int lo, Int hi)` | `Int` | Integral: **inclusive** both ends, swaps if reversed |
| `range(float/double lo, hi)` | `float`/`double` | **Half-open** `[lo, hi)` |
| `real()` / `real_d()` | `float` / `double` | `[0, 1)` using 24 / 53 mantissa bits |
| `chance(double p)` | `bool` | `false` for `p <= 0`, `true` for `p >= 1` |
| `coin()` | `bool` | Raw low bit |
| `pick(Container&)` | element (by value) | Uniform pick; empty container returns a static default-constructed element |
| `shuffle(first, last)` | — | Fisher–Yates |
| `weighted(weights)` | `size_t` | Index by weight; all-zero/negative falls back to uniform |
| `unitVector()` | `Vector2f` | Uniform direction on the unit circle |
| `inCircle(float r)` | `Vector2f` | Uniform inside a disk (`sqrt` scaling) |
| `onCircle(float r)` | `Vector2f` | Uniform on a circle |
| `inRect(const SDL_Rect&)` | `Vector2f` | Uniform inside a rectangle |
| `normal(mean = 0, stddev = 1)` | `double` | Box–Muller, keeps one spare value |
| `roll(count, sides)` | `long long` | Sum of `count` dice with `1..sides`; `0` for non-positive inputs |
| `hash(int x, int y, uint64_t seed)` | `uint64_t` | Deterministic per-tile hash |

Caveats: `pick` returns by value (`auto` decays the reference), and `split()` is declared `const` but derives its seed through a `const_cast` of `m_seed`, so calling it on a genuinely `const Random` is undefined behavior.

---

## 10. Camera — `camera.h`

The camera never mutates world data. `Camera` owns the follow state; `CameraView` is a per-frame snapshot that converts world coordinates to screen coordinates and clamps to level bounds. All world drawing goes through it.

### 10.1 `Camera`

| Member | Default | Notes |
| --- | --- | --- |
| `target` | `{}` | World point to follow |
| `pos` | `{}` | Smoothed camera center, world space |
| `deadzone` | `{80, 60}` | Half-size of the box the target can move inside without moving the camera (`Game::init` overrides it to `{0, 0}`) |
| `worldMin` / `worldMax` | `{}` | Level bounds; set equal to disable clamping |
| `viewSize` | `{1280, 720}` | Screen size in pixels (`Game::init` sets it to `SCREEN_DIMENSION`) |
| `zoom` | `1.0f` | Screen pixels per world unit |
| `follow` | `6.0f` | Smoothing rate; higher is snappier |

`update(dt)` moves `pos` toward `target` one deadzone edge at a time, then applies exponential smoothing (`1 - exp(-follow * dt)`), so follow speed is framerate-independent. Call it once per frame after gameplay has moved the target. `pos` is a center point, not a top-left corner.

### 10.2 `CameraView`

`CameraView view = CAMERA;` (or assign in the frame loop) captures the transform for one frame.

| Member | Notes |
| --- | --- |
| `origin` | World position of the screen's top-left corner |
| `size` | Screen size in pixels |
| `visible` | Visible world size (`size / zoom`), handy for culling |
| `zoom` | Copied from `Camera` |

Construction clamps `origin` to `[worldMin, worldMax - size / zoom]` on each axis when the bounds differ. API:

| Method | Returns | Notes |
| --- | --- | --- |
| `toScreen(const SDL_FRect&)` | `SDL_FRect` | Scales position and size by `zoom` |
| `toScreen(Vector2f)` | `Vector2f` | Point conversion |
| `toWorld(Vector2f)` | `Vector2f` | Inverse of `toScreen`; use for mouse-to-world |
| `visibleRect(const SDL_FRect&)` | `bool` | Cheap AABB cull against the visible region |

### 10.3 `drawSprite`

```cpp
drawSprite(RENDERER, texture, &src, worldRect, VIEW);        // pixel art: snapped
drawSprite(RENDERER, texture, &src, worldRect, VIEW, false); // smooth/sub-pixel
```

`drawSprite` culls, transforms, optionally snaps to whole pixels (`UI::snap`) and calls `SDL_RenderCopy`. It is the intended path for world objects; draw UI separately through `UI::Draw` (screen space, unaffected by the camera). Pass `pixelSnap = false` if a sprite should move at sub-pixel precision; snapping avoids shimmer on pixel art.

---

## 11. Invariants & gotchas

1. **One translation unit.** Only `src/main.cpp` is compiled; it includes `game.cpp`.
2. **Init order matters.** Renderer → `CAMERA` config → `ResourceManager::init` → `UI::init` → `UI::Draw::initNineSlice` → `SoundManager::init`. Shutdown is the reverse, and `UI::kill()` must precede `SDL_DestroyRenderer()`.
3. **Camera state is snapshot per frame.** `CAMERA.update(dt)` runs after gameplay and `VIEW = CameraView(CAMERA)` runs once before drawing; do not rebuild `VIEW` mid-frame or earlier draws and hit-tests will disagree.
4. **Draw before present.** `UI::endFrame()` and `Input::lateUpdate()` run after `SDL_RenderPresent()`; rendering issued after present appears a frame later.
5. **Widget IDs share one map.** Duplicate IDs across widget types corrupt each other's hover/press/text state.
6. **Hit-testing uses base rects plus the current transform offset.** Widgets drawn inside a `Draw::push` are clickable where they appear, while their own squish animation cannot feed back into hover.
7. **Textures are shared through the cache.** Anything that changes texture state (`alpha mod`, blend mode) must restore it — `Draw::copy` resets alpha to 255 for that reason.
8. **A theme must outlive its use.** `setTheme` stores a pointer; passing a local `Theme` dangles.
9. **Asset tables are compile-time checked.** Adding an ID without a path (or vice versa) fails the `static_assert`.
10. **Single music track.** SDL_mixer streams one `Mix_Music` at a time; `isMusicPlaying()` is also true while paused.
11. **`ensurePanelRect` / `panelRect[9]` / `PANEL_BORDER` are legacy.** The current nine-slice path queries the texture itself and ignores them.
12. **`util/randomizer.h` is not wired in.** Include it manually where you need RNG; `input.h` pulls in `util/vector2.hpp` transitively.
13. **World space vs screen space.** `drawSprite` applies `VIEW`; `UI::Draw` does not. Do not feed world coordinates to UI widgets or raw coordinates to `drawSprite`.
