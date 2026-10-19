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

Hold Tab to fast-forward (up to 4x). With a gamepad, the right stick moves a stylus pointer over the touch screen and
either trigger touches. Everything can be rebound in the settings (F1, or R + Start on a gamepad). Other keys: F2 swaps the screens, F3 shows or
hides the second screen (in One screen: the corner screen's size), F10 cycles the layout (single screen with a
picture-in-picture, main + side, stacked, side by side, separate window, one screen), F5 switches the renderer, F6/F7 lower or raise the resolution, F8 cycles the filter, F11 toggles
fullscreen.

## PC features

- Layouts: side by side (default), stacked, main screen + side, single screen with the other as picture-in-picture,
  or each screen in its own window (F10 cycles them, F2 swaps the screens).
- One screen (16:9): the game's two screens are two halves of one view of the world, so they are joined into one
  widened picture that fills a 1920x1080 display. Where only the bottom screen has 3D (shops, classrooms) it fills the
  window alone, with the top screen as a corner picture that grows for a moment when it changes (new text); menus show
  both screens 4:3 side by side. The arrangements move into each other as you go from place to place.
- High resolution: the OpenGL renderer draws at up to 16x the DS's resolution (auto matches the window), with optional
  texture smoothing. The software renderer is the exact, native one.
- Widescreen on both screens (16:9, 16:10, 21:9, 32:9): more of the world to the sides; 2D screens stay 4:3.
- Zoom: where one screen fills the window (rooms, shops, classrooms), the camera comes in just enough that the wide
  picture is filled with what the game really draws (the game builds each place only as wide as the DS's screen).
  The game's own projection is narrowed, so its culling and the stylus on the 3D world follow. Where both screens
  join outdoors the view stays as wide as it is: zooming each screen about its own middle would break the join.
- 60 fps 3D on both screens: the DS draws its 3D on one screen per frame (30 fps each); a frame is blended in
  between on each.
- Smooth 2D: text, menus, icons and sprites are upscaled with their diagonal steps smoothed instead of as blocks.
- Upscale textures: the game's textures enlarged 4x with Scale2x applied twice (crisp edges, smooth diagonals).
- Texture packs: dump the game's textures (Settings > Video > Dump textures) to `textures/dump` in the data folder,
  edit or redraw them at 1, 2 or 4 times their size, and put them in `textures/load` with the same names.
- Walk speed (Normal, 125%, 150%, twice as fast): only your character, the rest of the game keeps its pace.
- Text speed (Normal, Fast, Instant), quick fades between places, and hold Tab to fast-forward. F12 saves a
  screenshot to `screenshots` in the data folder. The game pauses while its window is in the background (an
  option), and the mouse pointer hides after two seconds without moving.
- Cheats: move the game's clock days or hours ahead or back (Settings > Game > Cheats).
- Keyboard and gamepad, all rebindable; the mouse is the stylus, also through a widened or picture-in-picture screen.
- Fast loading and a faster ARM9 (2x/4x) for smoother busy scenes; an in-game settings menu (F1).
- The game's clock is your PC's: days, hours and seasons pass with the real date and time, as on a DS (its own
  clock keeps going while the game is paused or fast-forwarded).


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
- [x] Through the first day: the dormitory, the walk to school, the entrance ceremony and the first class, in random
      play-throughs of 60,000 frames without a crash; the pictures match a plain emulator (`tools/mqref`) given the
      same input frame for frame (`tools/mq/compare_ref.py`)
- [x] Windows build tested (the package from `main` runs on Windows)
- [x] Widescreen on both screens (OpenGL; Settings > Aspect ratio): the game's projection is widened, and since it
      shows its 3D on both screens by alternating them through 256-wide display capture, each screen's live wide
      picture is kept and shown in place of the captured copy. Dialogue boxes and the HUD stay in their 4:3 places; 2D-only screens keep black sides.
- [x] Fast loading (on by default) and the ARM9 at 2x/4x (the runtime boots at 1x, then switches)
- [x] One screen (16:9): both screens joined into one widened picture; the bottom screen alone where only it has
      3D, with the top as a corner picture; menus side by side. The game builds only what a 4:3 view shows (its box
      tests are not what limits it), so empty parts of the wide sides continue the scene beside them, darkened.
      Arrangement changes wait for a scene to settle (and stay put through dark loads); a lone 2D screen sits over a
      dim copy of itself; the sides go black with the game's black curtains and dim beside text boxes.

## Download (Windows)

Every push to `main` builds a ready-to-run Windows package, published as the rolling **latest** pre-release:
[MagiciansQuest-Recomp-0.1.0-alpha-windows-x64.zip](https://github.com/waifoof/MagiciansQuest-Recomp/releases/download/latest/MagiciansQuest-Recomp-0.1.0-alpha-windows-x64.zip)
(also the **MagiciansQuest-windows** artifact of each windows-package run under the Actions tab). Extract it somewhere you can write to and
run `MagiciansQuest.exe`; choose your ROM on the start screen and the game is built from it once with the compiler that
comes in the package. The program is unsigned, so SmartScreen may warn the first time (More info, Run anyway).

## Building on Windows

```powershell
powershell -ExecutionPolicy Bypass -File tools\windows\build.ps1
```

It installs what is missing with winget (Git, CMake, Ninja, LLVM, Python, the Visual Studio C++ Build Tools), clones
the repository if needed and builds `build\release\game\MagiciansQuest.exe`. Start it, choose your ROM, and it builds the game
once. Saves, settings and logs (`logs\game.log`): `%APPDATA%\MagiciansQuest-Recomp\MysteriousTimes`.

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
