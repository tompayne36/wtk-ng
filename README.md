# WTK-NG

WTK-NG is a source-compatibility runtime for applications originally written
against Sense8 WorldToolKit. It provides the subset of the classic C API needed
by Space Rocks using SDL2, OpenGL, and libjpeg on modern systems.

The project is an independent clean-room compatibility implementation. It does
not contain the proprietary WorldToolKit runtime.

## Current capabilities

- Scene graph and movable nodes
- NFF geometry loading
- TGA and JPEG textures
- Fixed-function OpenGL rendering
- Viewpoints, overlays, keyboard, and mouse input
- Tasks and animation callbacks
- Collision helpers
- SDL-based WAV playback and mixing
- Native macOS, Windows (MinGW), and Emscripten support

Several broad legacy APIs—specialized VR hardware, native WTK dialogs,
networking, and full spatial audio—are currently compatibility stubs.

## Requirements

- A C11-compatible compiler
- SDL2
- libjpeg
- OpenGL on native builds

## Build

```sh
make
```

This produces `libwtk-ng.a`. Applications include `wt.h` and link the library
along with SDL2, libjpeg, OpenGL, and libm.

Space Rocks is the reference application and integration test.

### Windows

Use the **UCRT64** shell from [MSYS2](https://www.msys2.org/). Install:

```sh
pacman -S --needed make mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-SDL2 \
  mingw-w64-ucrt-x86_64-libjpeg-turbo
make
```

This builds a native Windows `libwtk-ng.a`. Applications link with
`pkg-config --libs sdl2 libjpeg`, `-lopengl32`, and `-lm`. Distribute SDL2,
libjpeg, and any imported compiler runtime DLLs with the executable.
Space Rocks includes a portable packaging script for its assets and DLLs.

## Automated builds

The **Native builds** GitHub Actions workflow compiles the runtime on Windows
x64, macOS Apple Silicon, and macOS Intel on every push and pull request. It
can also be started manually from the repository's Actions tab.

Successful runs provide archives of `libwtk-ng.a`, `wt.h`, and this README as
downloadable artifacts, retained for 14 days. These are development libraries;
applications still need to link the platform dependencies described above.
The hosted jobs check compilation. Interactive graphics, input, and audio
should be tested on the target hardware.
