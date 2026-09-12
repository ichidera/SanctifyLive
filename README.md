# SanctifyLive

SanctifyLive is a church presentation program: an operator drives a schedule
of slides from a control window, and a separate, congregation-facing output
window (typically on a second display or projector) shows exactly what's
live — nothing more, nothing less.

It's built on Qt Widgets + CMake, with a deliberately small, modular C++
core so the project stays easy to reason about as features are added.

## Contents

- [Features](#features)
- [How it works](#how-it-works)
- [Project layout](#project-layout)
- [Building](#building)
  - [Windows](#windows-official-qt-installer)
  - [Windows (vcpkg)](#windows-qt-via-vcpkg)
  - [Linux](#linux-apt-based-eg-ubuntudebian)
  - [macOS](#macos-homebrew)
  - [Troubleshooting](#quick-diagnosis-checklist)
- [Roadmap](#roadmap)
- [License](#license)

## Features

Current milestone:

- **Schedule list** — add/remove plain-text slides, reorder by selecting.
- **Live control loop** — Next / Previous / Black, plus keyboard shortcuts
  (arrow keys, space, `B`), all driving a single source-of-truth model.
- **Two-window output** — a real output window (send it to a projector,
  full-screen) and an in-app live preview, both fed by the same model so
  they never disagree about what's live.
- **Resolution-independent rendering** — every slide is rendered once into
  a fixed logical canvas and scaled to whatever physical screen it lands
  on, so a 1080p confidence monitor, a 720p projector, and a small preview
  thumbnail all show pixel-faithful, correctly laid-out copies of the same
  frame. See [How it works](#how-it-works).

Deliberately absent for now (see [Roadmap](#roadmap)): song/Scripture
databases, media backgrounds, themes, multi-layer composition, second
control surfaces (web remote, stage view).

## How it works

The rendering pipeline follows one rule: **render once into a fixed
"design resolution," scale that result to whatever physical screen needs
it.** Nothing downstream of the renderer ever recomputes font sizes or
layout based on a window's actual pixel dimensions.

```
Slide (data)  →  SlideRenderer  →  QPixmap @ design resolution (1920x1080)
                                        │
                    ┌───────────────────┼───────────────────┐
                    ▼                   ▼                   ▼
             Output window        Live preview        (future: stage view,
             (any projector,      (small embedded         web remote, ...)
              any resolution)      thumbnail)
```

The pixmap is only re-rendered when the live slide actually changes —
resizing a window, moving it to a different monitor, or shrinking it to a
thumbnail is purely a scaling operation and never touches the renderer.
This is the same technique used by video/broadcast tooling and is what
keeps output correct and cheap regardless of how many screens or preview
surfaces are watching the same model.

## Project layout

```
src/
├── main.cpp
└── core/
    ├── model/     # Plain data + state: Slide, ScheduleModel.
    │              # No Qt widgets, no rendering — the single source of
    │              # truth for "what is the service doing right now."
    │
    ├── render/    # Turns model data into pixels: SlideRenderer, and the
    │              # design-resolution contract (RenderResolution.h).
    │              # No knowledge of physical windows or screens.
    │
    └── ui/        # Qt widgets: OutputWindow, OperatorWindow.
                   # No slide-drawing logic of its own — only asks
                   # render/ for pixmaps and displays them.
```

Dependency direction is one-way: `ui/` → `render/` → `model/`. Nothing in
`model/` or `render/` ever includes anything from `ui/`, which is what
lets a future headless mode, CLI, or alternate front-end reuse the same
model and renderer without dragging in Qt widget code.

## Building

The #1 cause of build failures is CMake not knowing *where* Qt is
installed. Having Qt on your machine is not enough — you must point
`CMAKE_PREFIX_PATH` at the specific Qt "kit" folder (the one containing
`bin/`, `lib/`, `include/`).

Requirements: Qt Widgets (Qt5 or Qt6) + CMake + a C++17 compiler.

### Windows, official Qt installer

You installed Qt to `X:\Qt\6.8.0\msvc2022_64`. Two ways to build:

#### Option A — use Qt's own `qt-cmake` wrapper (easiest)

It auto-fills `CMAKE_PREFIX_PATH` for you:

```bat
cd X:\Github\SanctifyLive
rmdir /s /q build
mkdir build && cd build
X:\Qt\6.8.0\msvc2022_64\bin\qt-cmake.bat ..
cmake --build . --config Release
```

#### Option B — plain cmake, pass the prefix path yourself

```bat
cd X:\Github\SanctifyLive
rmdir /s /q build
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="X:\Qt\6.8.0\msvc2022_64"
cmake --build . --config Release
```

**Important:** with the Visual Studio generator (the default on Windows),
`make` is not a valid command — use `cmake --build . --config Release`
instead. If you'd rather use `make`/`mingw32-make` directly, pass
`-G "Ninja"` (fastest) or `-G "MinGW Makefiles"` at the configure step and
use a matching Qt kit (e.g. `mingw_64`, not `msvc2022_64`).

After building, deploy the Qt DLLs next to the exe (only needed for
running outside Qt Creator / distributing the app):

```bat
X:\Qt\6.8.0\msvc2022_64\bin\windeployqt.exe --release ^
    X:\Github\SanctifyLive\build\Release\SanctifyLive.exe
```

#### Option C — set it once, globally

Instead of passing it every time, set an environment variable so CMake
finds Qt automatically in every project:

```bat
setx CMAKE_PREFIX_PATH "X:\Qt\6.8.0\msvc2022_64"
```

(open a new terminal afterwards, then just `cmake ..` works.)

### Windows, Qt via vcpkg

```bat
vcpkg install qtbase
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake"
cmake --build . --config Release
```

### Linux (apt-based, e.g. Ubuntu/Debian)

```bash
sudo apt-get install qtbase5-dev cmake build-essential   # Qt5
# or: sudo apt-get install qt6-base-dev cmake build-essential  (Qt6)

mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
./SanctifyLive
```

System-installed Qt on Linux is normally found automatically — no
`CMAKE_PREFIX_PATH` needed.

### macOS (Homebrew)

```bash
brew install qt cmake
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build . -j$(sysctl -n hw.ncpu)
./SanctifyLive.app/Contents/MacOS/SanctifyLive
```

### Quick diagnosis checklist

| Symptom | Fix |
|---|---|
| `Could not find a package configuration file provided by "QT"` | Qt location not on `CMAKE_PREFIX_PATH` — see Option A/B above |
| `make` says nothing to do / not recognized (Windows) | You have VS project files, not Makefiles — use `cmake --build . --config Release`, or reconfigure with `-G "Ninja"` |
| App runs but window is blank/missing platform plugin | Run `windeployqt` (Windows) or ensure `QT_QPA_PLATFORM_PLUGIN_PATH` is set, so the app finds `platforms/qwindows.dll` etc. |
| Mixing MSVC-built Qt with MinGW compiler (or vice versa) | The Qt kit and compiler toolchain must match — `msvc2022_64` kit needs MSVC, `mingw_64` kit needs MinGW |

## Roadmap

Rough order, each item building on the render/model/ui separation above
without requiring structural rewrites:

- [ ] Group slides into higher-level schedule items (a whole song, a whole
      reading) instead of a flat slide list
- [ ] Background media (image/video) per slide
- [ ] Text styling: font, size, color, outline, per-theme presets
- [ ] Multi-layer composition (background + text + lower-third)
- [ ] Song/Scripture lookup and import
- [ ] Additional control surfaces (web remote, stage/confidence view)
      reading from the same `ScheduleModel`

## License

Licensed under the [Apache License 2.0](LICENSE).

Copyright © 2026 [ichidera](https://github.com/ichidera)
