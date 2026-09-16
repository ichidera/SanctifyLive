# SanctifyLive

SanctifyLive is a church presentation program: an operator drives a schedule
of slides from a control window, and a separate, congregation-facing output
window (typically on a second display or projector) shows exactly what's
live — nothing more, nothing less.

It's built on Qt Widgets + CMake, with a deliberately small, modular C++
core so the project stays easy to reason about as features are added.

## Contents

- [Features](#features)
- [Panel glossary](#panel-glossary)
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

- **Operator console layout** — a dark, broadcast-console-style window,
  closer to OpenLP's Service Manager / Preview / Live arrangement than a
  single stacked list: a top toolbar (session actions + the live
  transport), a main row of **Schedule / Preview / Live** panels side by
  side, and a bottom content strip (Songs / Scriptures / Media /
  Presentations / Themes tabs) alongside **History** (a log of what's
  actually gone live) and **Transcription** (a placeholder for now).
- **Preview vs. Live** — clicking a Schedule row stages it in the Preview
  pane without touching the congregation-facing output; double-clicking
  it (or using keyboard shortcuts) commits it live. Schedule starts
  empty and has no manual "type a slide" entry point on purpose — real
  items are meant to come from picking actual content (Media today;
  Songs/Scriptures/Presentations once they exist), not free-typed text.
- **History** — an append-only, most-recent-first log of every slide
  that's actually gone live, timestamped, independent of the Schedule
  (which is the plan, not the record).
- **Live control loop** — Black / Clear, plus keyboard shortcuts (arrow
  keys, space to advance/retreat, `B` for black), all driving a single
  source-of-truth model. There's no dedicated Previous/Next button pair
  in the UI anymore; the "Next: ..." readout under the Main Row is
  informational only.
- **Two-window output** — a real output window (send it to a projector,
  full-screen) and an in-app live preview, both fed by the same model so
  they never disagree about what's live.
- **Resolution-independent rendering** — every slide is rendered once into
  a fixed logical canvas and scaled to whatever physical screen it lands
  on, so a 1080p confidence monitor, a 720p projector, and a small preview
  thumbnail all show pixel-faithful, correctly laid-out copies of the same
  frame. See [How it works](#how-it-works).
- **Save/Open schedules** — a service's slide queue can be saved to and
  loaded from a `.json` file (`core/model/ScheduleIO`), independent of
  the UI that triggers it.
- **Item Preview** — a pixel-faithful render of whatever's currently
  selected/hovered in the content tabs (Media today; more tabs later),
  *before* it's added to the Schedule. Lives next to History, not
  inside any one content tab, since it's meant to serve all of them.
- **Media tab** — Videos and Images are genuinely separate collections
  (own grid, own item count, own **+** import filter, switched via the
  folder tree), not one grid pretending to be two. Images ships six
  built-in solid-color swatches standing in for a real library; clicking
  one renders it in the Item Preview panel exactly as it would appear
  live (same SlideRenderer/SlideCanvas pipeline as Schedule/Preview/
  Live), and double-clicking really does add a slide using that color.
  The bottom bar's **+** button goes straight to a file picker — no
  "what kind of file?" menu, since the selected folder already answers
  that — filtered to real image extensions on Images, real video
  extensions on Videos. Imported images carry their real file path
  end to end (`Slide::backgroundImagePath`): the thumbnail shown while
  browsing, the Item Preview render, and a live slide (if activated) are
  all the same actual photo, cover-fit by `SlideRenderer` — not a color
  approximation. Imported videos are cataloged (real file, real name)
  but not interactive — no thumbnail (no decoder wired in), and
  clicking/double-clicking does nothing, since there's no honest "this
  is what it'll look like live" answer to give for video yet; that's
  planned to need real VLC/FFmpeg-level work down the line. Settings and
  View are honest disabled stubs, same pattern as Transcription/
  Profiles. The item count reflects whichever folder is currently
  showing, read live off its grid, never hardcoded. **Imports persist
  across restarts** (`core/storage/MediaLibraryStore`): the catalog of
  what's been imported (name, file path, kind) is saved to a small JSON
  file in the app's data directory and reloaded on next launch, so
  opening SanctifyLive doesn't reset the library back to just the
  built-in swatches. Files themselves are referenced from wherever the
  operator picked them, not copied into app storage; if a remembered
  file has since moved or been deleted, it's dropped from the library on
  load (with a warning logged) instead of showing a broken tile.

Deliberately absent for now (see [Roadmap](#roadmap)): song/Scripture
databases, real media import, themes, multi-layer composition, live
transcription, second control surfaces (web remote, stage view), and the
toolbar's Web / Remote / Alerts / Logo actions, which are present as
disabled stubs so the layout matches the target design but don't claim
to do anything yet.

## Panel glossary

Every named box in the operator window has exactly one canonical name,
used consistently in code, comments, and issue/PR discussion. The full,
authoritative version of this glossary lives as a doc comment at the top
of `src/ui/OperatorWindow.h` — treat this table as a quick-reference
summary, and that header as the source of truth if the two ever drift.

| Name | What it is | Code |
| --- | --- | --- |
| **Toolbar** | Top bar: New/Open/Save, Web/Remote (stubs), Go Live, Alerts/Logo (stubs), Black, Clear, the LIVE/Offline indicator. | `OperatorWindow::buildToolBar()` |
| **Menu Bar** | File/Live/Profiles/View/Help — every entry duplicates a Toolbar/panel action. | `OperatorWindow::buildMenuBar()` |
| **Schedule** | The run order for the service (OpenLP calls its equivalent the "Service Manager"). Single click stages Preview; double-click/keyboard shortcuts commit Live. Starts empty — no manual "type a slide" entry point. | `OperatorWindow::buildSchedulePanel()`, `m_scheduleList` |
| **Preview** | Whichever Schedule row is currently *selected* — staged, not yet live. A *service* preview. | `m_previewCanvas` |
| **Live** | Mirrors the model's actual live position at all times, in-app — independent of whether the real Output Window is showing it. | `m_livePreview` (an embedded `OutputWindow`) |
| **Transport Row** | Just the "Next: ..." readout now — informational only, no button pair. | `OperatorWindow::buildTransportRow()` |
| **Content Tabs** | Songs/Scriptures/Media/Presentations/Themes — where the operator browses source material to add to Schedule. | `OperatorWindow::buildContentTabs()` |
| **Media panel** | The Media tab's own contents: folder tree + separate Images/Videos grids + bottom bar (add/settings/count/view). | `MediaLibraryPanel` |
| **Item Preview** | Pixel-faithful render of whatever's selected/hovered in Content Tabs, *before* it's added to Schedule. A *library-content* preview — not the same thing as Preview above. | `OperatorWindow::buildItemPreviewPanel()`, `m_itemPreviewCanvas` |
| **History** | Read-only, append-only, most-recent-first log of every slide that's actually gone live, timestamped. | `OperatorWindow::buildHistoryPanel()`, `m_historyList`, `ScheduleModel::history()` |
| **Transcription** | Placeholder stub — no speech-to-text pipeline exists yet. | `OperatorWindow::buildTranscriptionPanel()` |
| **Status Bar** | Bottom strip: output state + slide count, and the "Wake Display" button. | Built inline in `OperatorWindow`'s constructor |
| **Output Window** | The actual, possibly-fullscreen, congregation-facing display (what a projector shows). | `OutputWindow`, `m_outputWindow` |

Two names are deliberately similar and worth double-checking before
using either one: **Preview** (Main Row, stages the next *service* item)
vs. **Item Preview** (Lower Area, previews *library content* before it's
even added to the Schedule).

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
├── core/                    # Everything that ISN'T a widget. No <QWidget>
│   │                        # includes anywhere under core/.
│   ├── model/               # Plain data + state: Slide, ScheduleModel,
│   │   │                    # and ScheduleIO (JSON save/load). The
│   │   │                    # single source of truth for "what is the
│   │   │                    # service doing right now" — safe to reuse
│   │   │                    # from a future headless/CLI mode.
│   │   ├── Slide.h
│   │   ├── ScheduleModel.h / .cpp
│   │   └── ScheduleIO.h / .cpp
│   │
│   └── render/               # Turns model data into pixels: SlideRenderer,
│       │                     # and the design-resolution contract
│       │                     # (RenderResolution.h). No knowledge of
│       │                     # physical windows or screens.
│       ├── RenderResolution.h
│       └── SlideRenderer.h / .cpp
│
└── ui/                      # Qt widgets: the operator console and
    │                        # everything it's built from. No slide-
    │                        # drawing logic of its own — only asks
    │                        # core/render for pixmaps and displays them.
    ├── OperatorWindow.h / .cpp    # the volunteer-facing control surface
    ├── OutputWindow.h / .cpp      # the congregation-facing output
    ├── SlideCanvas.h / .cpp       # shared scaled-pixmap display widget
    ├── MediaLibraryPanel.h / .cpp # the Media tab: folder tree + Images/Videos grids
    └── Theme.h / .cpp             # the app-wide dark stylesheet
```

`ui/` lives as a top-level sibling of `core/`, not nested inside it — the
UI is a consumer of the core, not a subdirectory of it. Dependency
direction is one-way: `ui/` → `core/render/` → `core/model/`. Nothing
under `core/` ever includes anything from `ui/`, which is what lets a
future headless mode, CLI, or alternate front-end (or a totally different
UI toolkit) reuse the same model and renderer without dragging in Qt
widget code.

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
- [x] Image backgrounds per slide (`Slide::backgroundImagePath`, cover-fit
      via `SlideRenderer`) — video backgrounds are still unbuilt
- [ ] Real video decode/thumbnail/playback (VLC/FFmpeg-level work) for
      the Videos folder in the Media tab
- [ ] Text styling: font, size, color, outline, per-theme presets
- [ ] Multi-layer composition (background + text + lower-third)
- [ ] Song/Scripture lookup and import
- [ ] Live transcription (speech-to-text feeding the Transcription panel)
- [ ] Additional control surfaces (web remote, stage/confidence view)
      reading from the same `ScheduleModel`
- [ ] Wire up the toolbar's Web / Remote / Alerts / Logo actions
      (currently disabled stubs) once their backing features exist

See `CHANGELOG.md` for what's landed release by release.

## License

Licensed under the [Apache License 2.0](LICENSE).

Copyright © 2026 [ichidera](https://github.com/ichidera)
