# MagiciansQuest-Recomp

A native PC port of **Magician's Quest: Mysterious Times** (Nintendo DS, Konami, 2009), made by statically recompiling
the game: its ARM code is translated to C and compiled on your PC, not emulated.

> **Status: early bring-up. Not playable yet.** The game boots and runs its start-up code; work is on getting it to
> draw its first screens. See [Progress](#progress).

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
- [ ] First screens (logos, title)
- [ ] New game, town, school
- [ ] Windows build

## Building (development, Linux)

```sh
git clone --recursive https://github.com/waifoof/MagiciansQuest-Recomp.git
cd MagiciansQuest-Recomp
cmake --preset linux-release -DFFCC_ROM="path/to/your/dump.nds"
cmake --build --preset linux-release
build/linux-release/game/ffcc "path/to/your/dump.nds" --cpu-speed 1
```

The recompiled C goes to `build/` and is never committed. Run `python tools/check_no_game_data.py --install-hook` before
your first commit.

## Legal

Magician's Quest: Mysterious Times is © Konami. This project is not affiliated with or endorsed by Konami or Nintendo.
Licensed under GPL-3.0-or-later, like the framework it is built on; see [LICENSE](LICENSE).
