# dum-gippity

A tiny **SDL3-based Terraria-like sandbox prototype** built with CMake and C++23.

## Features

- Procedurally generated 2D tile world (grass / dirt / stone)
- Basic player movement with gravity + jumping
- Camera that follows the player
- Mouse block editing:
  - **Left click** removes blocks
  - **Right click** places dirt blocks

## Controls

- `A` / `D`: move left / right
- `Space`: jump
- `Esc`: quit
- Left click: mine block
- Right click: place block

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/dum_gippity
```

## Notes

- CMake tries `find_package(SDL3)` first.
- If SDL3 is not installed locally, it will fetch SDL3 from GitHub automatically during configure.
