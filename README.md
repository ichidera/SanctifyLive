# SanctifyLive
A Presentation Software for Churches

## Incase you want to join in development

We welcome contributions from developers who want to help improve SanctifyLive.

If you'd like to get involved:

- Fork the repository and create a feature branch.
- Review the project structure and existing code in `src/` and `scripts/`.
- Build and test locally using the project instructions in the repo.
- Submit a pull request with a clear description of your changes.

Whether you're interested in fixing bugs, improving the UI, adding features, or helping with documentation, your contributions are welcome.

### Getting Started (for Developers)
#### Prerequisites

Before building SanctifyLive, make sure you have the following installed:

- A working C/C++ toolchain, such as `gcc` or `clang`
- Build tools like `make`, `cmake`, and a compatible native build environment
- Qt development libraries and tools (Qt 5 or Qt 6 depending on your setup)
- A Git client for cloning and contributing to the project
- Optional platform dependencies for Android or other target builds

#### Build Instructions

##### Windows (PowerShell)

From the project root:

```powershell
cmake -B build -DCMAKE_PREFIX_PATH="pathtoQT\Qt\6.8.0\msvc2022_64"   # if Qt is not already on your PATH
cmake --build build --config Release --parallel

& "path-to-qt\Qt\6.8.0\msvc2022_64\bin\windeployqt.exe" .\build\Release\SanctifyLive.exe

.\build\Release\SanctifyLive.exe
```

If Qt is already installed and available in your environment, you can omit the `-DCMAKE_PREFIX_PATH` argument.

##### Linux

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/SanctifyLive
```

If Qt is not in your default path, set `CMAKE_PREFIX_PATH` to your Qt installation directory:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/path/to/Qt/6.8.0/gcc_64"
cmake --build build --parallel
./build/SanctifyLive
```

##### macOS

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/path/to/Qt/6.8.0/clang_64"
cmake --build build --parallel
./build/SanctifyLive
```

On macOS, you may also need to ensure the Qt binaries are available in your shell environment before running the app.
