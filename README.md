# MagiciansQuest-Recomp

A native PC port of **Magician's Quest: Mysterious Times** (Nintendo DS, Konami, 2009), made by statically recompiling
the game: its ARM code is translated to C and compiled on your PC, not emulated.

> **Status: early alpha.** The game boots, a new game goes through character creation, and you arrive in the
> school dormitory in control of your character (3D field, dialogue, HUD), with sound, touch and buttons. Later
> parts of the game are untested.
> See [Progress](#progress).

This project is built on [FFCCEOT-Recomp](https://github.com/JonathanDC64/FFCCEOT-Recomp), the PC port of Final Fantasy
Crystal Chronicles: Echoes of Time, which uses the same NitroSDK and supplies the recompiler (`ntr-recomp`), the DS
runtime around [melonDS](https://github.com/melonDS-emu/melonDS), and the PC frontend. Its original README is kept in
[docs/FFCC_README.md](docs/FFCC_README.md).

## You need your own copy of the game

This repository contains **no** game code or data, only addresses (where functions start) and our own source. You
supply your own dump; the program builds the game from it on your PC.

| Region | Code | Dump | SHA-1 |
|---|---|---|---|
| USA | YNNE | `Magician's Quest - Mysterious Times (USA) (En,Fr,Es).nds` | `ab2e495f6206b5c9b69043f8b4114d760f7298b2` |

The European release (*Enchanted Folk and the School of Wizardry*) is not supported yet.

## Controls

| DS | Keyboard | Gamepad (Nintendo positions) |
|---|---|---|
| D-pad | Arrow keys | D-pad, left stick |
| A / B / X / Y | X / Z / S / A | Right / bottom / top / left face button |
| L / R | Q / W | Shoulder buttons |
| Start / Select | Enter / Right Shift | Start / Back |
| Touch screen | Mouse (click and drag on the touch screen) | |

Everything can be rebound in the settings (F1, or R + Start on a gamepad). Other keys: F2 swaps the screens, F3 shows or
hides the second screen, F10 cycles the layout (single screen with a picture-in-picture, main + side, stacked, side by
side, separate window), F5 switches the renderer, F6/F7 lower or raise the resolution, F8 cycles the filter, F11 toggles
fullscreen.

## Planned PC features

From the framework (already in the frontend, untested on this game yet): single-screen play with the second screen as
picture-in-picture, stacked or side-by-side layouts, high resolution (OpenGL renderer up to 16x), texture smoothing,
rebindable keyboard and gamepad, mouse as the stylus, an in-game settings menu.

To do for this game: widescreen (the game's projection and culling hooked), gamepad controls for touch-only menus,
faster loading and saving.

## Progress

- [x] ROM recognised; code modules (ARM9, ITCM, DTCM, 39 overlays) extracted
- [x] Function seeds found by scanning calls and code pointers (`tools/mq/find_functions.py`), refined by run-time misses
- [x] NitroSDK thread switches and IRQs hooked (same SDK as FFCC)
- [x] Konami's own stack-switching task system runs on host fibers (`ntr_task_swap` / `ntr_task_load`)
- [x] Boots past the ARM7 handshake (at the DS's own CPU clock)
- [x] First screens: logos, title, menu (the dual-screen 3D swap needed exact `OS_SpinWait` and a GXSTAT catch-up)
- [x] New game: intro dialogue, name and birthday keyboards
- [x] Player setup: the start screen builds the game from your ROM (tested on Linux)
- [x] High resolution OpenGL renderer and single-screen + picture-in-picture layout work
- [x] Character creation (name, birthday, gender, appearance with the 3D model)
- [x] Into the game: the dormitory (Reginald's introduction), walking around
- [ ] Windows build tested (script ready: `tools/windows/build.ps1`)
- [ ] Widescreen: the game's projection is hooked (`mq_perspective_aspect`, behind `MQ_EXPERIMENTAL_WIDE`), but the
      game shows its 3D on both screens through display capture, which is 256 pixels wide: the renderer needs a wide
      capture path before the picture can be shown wide.
- [x] Fast loading (on by default) and the ARM9 at 2x/4x (the runtime boots at 1x, then switches)

## Building on Windows

```powershell
powershell -ExecutionPolicy Bypass -File tools\windows\build.ps1
```

It installs what is missing with winget (Git, CMake, Ninja, LLVM, Python, the Visual Studio C++ Build Tools), clones
the repository if needed and builds `build\release\game\ffcc.exe`. Start it, choose your ROM, and it builds the game
once. Saves and settings: `%APPDATA%\MagiciansQuest-Recomp\MysteriousTimes`.

## Building (development, Linux)

```sh
git clone --recursive https://github.com/waifoof/MagiciansQuest-Recomp.git
cd MagiciansQuest-Recomp
cmake --preset linux-release -DFFCC_ROM="path/to/your/dump.nds"
cmake --build --preset linux-release
build/linux-release/game/MagiciansQuest "path/to/your/dump.nds" --cpu-speed 1
```

The recompiled C goes to `build/` and is never committed. Run `python tools/check_no_game_data.py --install-hook` before
your first commit.

## Legal

Magician's Quest: Mysterious Times is © Konami. This project is not affiliated with or endorsed by Konami or Nintendo.
Licensed under GPL-3.0-or-later, like the framework it is built on; see [LICENSE](LICENSE).
