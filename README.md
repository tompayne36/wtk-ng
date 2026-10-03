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
- Native macOS and Emscripten support

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
