# SanctifyLive UI scaffold - build instructions

This is a pure interface scaffold (no live/streaming logic). The only
requirement is Qt Widgets (Qt5 or Qt6) + CMake + a C++17 compiler.

The #1 cause of build failures is CMake not knowing *where* Qt is installed.
Having Qt on your machine is not enough - you must point `CMAKE_PREFIX_PATH`
at the specific Qt "kit" folder (the one containing `bin/`, `lib/`, `include/`).

---

## Windows, official Qt installer (your case)

You installed Qt to `X:\Qt\6.8.0\msvc2022_64`. Two ways to build:

### Option A - use Qt's own `qt-cmake` wrapper (easiest)
It auto-fills `CMAKE_PREFIX_PATH` for you:

```bat
cd X:\Github\SanctifyLive
rmdir /s /q build
mkdir build && cd build
X:\Qt\6.8.0\msvc2022_64\bin\qt-cmake.bat ..
cmake --build . --config Release
```

### Option B - plain cmake, pass the prefix path yourself
```bat
cd X:\Github\SanctifyLive
rmdir /s /q build
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="X:\Qt\6.8.0\msvc2022_64"
cmake --build . --config Release
```

**Important:** with the Visual Studio generator (the default on Windows),
`make` is not a valid command - use `cmake --build . --config Release`
instead. If you'd rather use `make`/`mingw32-make` directly, pass
`-G "Ninja"` (fastest) or `-G "MinGW Makefiles"` at the configure step and
use a matching Qt kit (e.g. `mingw_64`, not `msvc2022_64`).

After building, deploy the Qt DLLs next to the exe (only needed for
running outside Qt Creator / distributing the app):
```bat
X:\Qt\6.8.0\msvc2022_64\bin\windeployqt.exe --release ^
    X:\Github\SanctifyLive\build\Release\SanctifyLive.exe
```

### Option C - set it once, globally
Instead of passing it every time, set an environment variable so CMake
finds Qt automatically in every project:
```bat
setx CMAKE_PREFIX_PATH "X:\Qt\6.8.0\msvc2022_64"
```
(open a new terminal afterwards, then just `cmake ..` works.)

---

## Windows, Qt via vcpkg

```bat
vcpkg install qtbase
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake"
cmake --build . --config Release
```

---

## Linux (apt-based, e.g. Ubuntu/Debian)

```bash
sudo apt-get install qtbase5-dev cmake build-essential   # Qt5
# or: sudo apt-get install qt6-base-dev cmake build-essential  (Qt6)

mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
./SanctifyLive
```
System-installed Qt on Linux is normally found automatically - no
`CMAKE_PREFIX_PATH` needed.

---

## macOS (Homebrew)

```bash
brew install qt cmake
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build . -j$(sysctl -n hw.ncpu)
./SanctifyLive.app/Contents/MacOS/SanctifyLive
```

---

## Quick diagnosis checklist

| Symptom | Fix |
|---|---|
| `Could not find a package configuration file provided by "QT"` | Qt location not on `CMAKE_PREFIX_PATH` - see Option A/B above |
| `make` says nothing to do / not recognized (Windows) | You have VS project files, not Makefiles - use `cmake --build . --config Release`, or reconfigure with `-G "Ninja"` |
| App runs but window is blank/missing platform plugin | Run `windeployqt` (Windows) or ensure `QT_QPA_PLATFORM_PLUGIN_PATH` is set, so the app finds `platforms/qwindows.dll` etc. |
| Mixing MSVC-built Qt with MinGW compiler (or vice versa) | The Qt kit and compiler toolchain must match - `msvc2022_64` kit needs MSVC, `mingw_64` kit needs MinGW |
