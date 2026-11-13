# MagiciansQuest-Recomp

A native PC port of **Magician's Quest: Mysterious Times** (Nintendo DS, Konami, 2009), made by statically recompiling
the game: its ARM code is translated to C and compiled on your PC, not emulated.

> **Status: feature-complete beta, close to 1.0.** The game plays on Windows and Linux (including the Steam Deck),
> with two-player co-op over a LAN or the internet: one shared town, mysteries, lessons, battles and hang-outs
> together. Everything is tested by long automated play on Linux, with simulated internet
> lag and packet loss. The first sessions between real PCs and a real Steam Deck are what is left before 1.0.
> See [Status](#status).

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

Hold Tab to fast-forward (up to 4x; not while playing together). With a gamepad, the right stick moves a stylus
pointer over the touch screen and either trigger touches; hold View and press RB to fast-forward, View + LB for a
screenshot (View alone is still Select). Everything can be rebound in the settings (F1, or R + Start on a gamepad).
Other keys: F2 swaps the screens, F3 shows or hides the second screen (in One screen: the corner screen's size), F10
cycles the layout (single screen with a picture-in-picture, main + side, stacked, side by side, separate window, one
screen), F5 switches the renderer, F6/F7 lower or raise the resolution, F8 cycles the filter, F11 toggles fullscreen,
F12 saves a screenshot.

## Playing together (co-op)

Two players, each with their own save, play in one town: the host's. It works over a LAN or the internet (tested up
to 200 ms round trip with packet loss). Windows and Linux / Steam Deck use the same protocol, so they can play
together.

1. Both start the game. On the start screen, choose **Co-op**.
2. One player picks **Host a game**, the other **Join a game**.
3. Swap the short **connect codes** the lobby shows: paste them, type them, or enter them with a controller. Each of
   you sees the other's name and ping.
4. The host presses **Play**. Both games load, and the guest arrives in the host's dorm.

No port forwarding is needed on most home routers: the host's game opens its port on the router by itself
(UPnP / NAT-PMP), and the connect codes let the two games reach each other through both routers. If they can't, the
lobby says so after 15 seconds and suggests forwarding UDP port 49251 or using Tailscale. On Windows, allow the game
through the firewall the first time you host. Settings > Network > Co-op between PCs still works for a LAN or a typed
address.

**What's shared:**

- **The town:** what either player changes in the host's town reaches the other within a second. The game's clock
  follows the host's, so days, shop hours and the night turn line up.
- **The people:** classmates live their normal lives in a session. Chats, gifts, friendship between the two students,
  and classmate hang-outs (your partner sees your classmate following you) all work.
- **Mysteries:** all 52 are played in a pair. When one player starts a scene, the other is asked "Go and help?" and
  joins it.
- **Lessons, battles, festivals:** you can take lessons in one classroom together. In battles you both fight the same
  monster (the host's), either player's hits count, and each keeps their own hearts. Festivals, holidays, the
  evaluation and the magic tournament work in a session too. So do every incantation, insects and fishing, shops, the
  café and the Gateway's destinations.
- **Each player's own progress:** lessons picked, talk limits and each console's own variables stay with that player,
  and are kept across a disconnect and rejoin. If the host leaves, the guest saves first.

The original game blocks most of this while its wireless is on; the port lifts those blocks one by one and keeps the
two games in step. Details: [docs/COOP.md](docs/COOP.md), [docs/COOP_INTERNET.md](docs/COOP_INTERNET.md); what was
played and found: [docs/COOP_PLAYTEST.md](docs/COOP_PLAYTEST.md). Optional: Settings > Network > Co-op status shows a
small line with where your partner is and the ping ("Partner: Café · 210 ms").

## Steam Deck (Linux)

The Linux build is a single AppImage with its own compiler inside, so setting the game up from your ROM needs nothing
installed:

- **Controller-only setup in Game Mode:** a built-in file browser finds the `.nds` file (home, Downloads, SD cards, USB
  drives), so a first start is Select ROM, A, A.
- **A layout for 1280x800:** on a Deck or a 16:10 display it starts with a large touch screen (800x600) and the top
  screen beside it (Settings > Video > Side screen size).
- **The touchscreen is the DS's touch screen** (through Steam's touch-as-mouse; still to be tried on a real Deck).
- **Crash reports** with a Restart / Report dialog, as on Windows.

Details and what has been tested: [docs/STEAM_DECK.md](docs/STEAM_DECK.md).

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
- Co-op between PCs: two players in one town over a LAN or the internet, from the first day of a new game (see
  [Playing together](#playing-together-co-op)).
- Co-op notices in the corner when your partner's game stops answering or comes back, and when the ping stays high;
  the window's title says how the session stands in plain words.
- Keyboard and gamepad, all rebindable; the mouse is the stylus, also through a widened or picture-in-picture screen.
- Fast loading and a faster ARM9 (2x/4x) for smoother busy scenes; an in-game settings menu (F1).
- Smooth fades at 60 fps (with frame interpolation), and the window comes back at its last size and place.
- The game's clock is your PC's: days, hours and seasons pass with the real date and time, as on a DS (its own
  clock keeps going while the game is paused or fast-forwarded).


## Status

- [x] The game recompiled and running natively: boot, menus, character creation, the 3D field, sound, touch and buttons
- [x] Played through by long automated runs (each 80,000 frames) whose pictures match a plain emulator frame for frame
      (`tools/mqref`, `tools/mq/compare_ref.py`); every change to `main` must reproduce them exactly
- [x] PC enhancements: high resolution, widescreen, one-screen 16:9 layout, 60 fps 3D, smooth 2D, texture packs,
      walk and text speed, fast loading
- [x] Windows package (zip with its own compiler) built and published on every push
- [x] Co-op over LAN and the internet: one town, all 52 mysteries, lessons, shared battles, festivals, incantations,
      classmate hang-outs; tested at 200 ms with 1% loss, over several in-game days, through the start-screen lobby
- [x] Connecting without router setup: UPnP / NAT-PMP, connect codes with hole punching, the co-op lobby
- [x] Linux AppImage for the Steam Deck: controller-only setup, 1280x800 layout, crash reports
- [x] The menus and lobby checked with a gamepad only at 1280x800
- [ ] First sessions on real hardware: a Windows PC with a Steam Deck over a real internet connection, the Deck's
      Game Mode and touchscreen
- [ ] The European release (*Enchanted Folk and the School of Wizardry*)
- [ ] PC view: one picture of the world from one camera instead of two DS screens (a prototype, off by default;
      [docs/PC_VIEW.md](docs/PC_VIEW.md))

Found a problem? Settings > About makes a problem report (a zip with the log and what's needed) and opens a filled-in
GitHub issue.

## Download

Every push to `main` builds ready-to-run packages, published as the rolling **latest** pre-release:

- **Windows:** [MagiciansQuest-Recomp-0.1.0-alpha-windows-x64.zip](https://github.com/waifoof/MagiciansQuest-Recomp/releases/download/latest/MagiciansQuest-Recomp-0.1.0-alpha-windows-x64.zip).
  Extract it somewhere you can write to and run `MagiciansQuest.exe`. The program is unsigned, so SmartScreen may warn
  the first time (More info, Run anyway).
- **Linux / Steam Deck:** [MagiciansQuest-Recomp-0.1.0-alpha-x86_64.AppImage](https://github.com/waifoof/MagiciansQuest-Recomp/releases/download/latest/MagiciansQuest-Recomp-0.1.0-alpha-x86_64.AppImage).
  Make it executable (`chmod +x`, or Properties > Permissions in Desktop Mode) and run it; add it to Steam as a
  non-Steam game to play it from Game Mode.

Choose your ROM on the start screen and the game is built from it once with the compiler that comes in the package.
Pick the **Enhanced** preset for the recommended setup (one 16:9 picture, 60 fps, smoothed text and textures). Saves,
settings and logs: `%APPDATA%\MagiciansQuest-Recomp\MysteriousTimes` on Windows,
`~/.local/share/MagiciansQuest-Recomp/MysteriousTimes` on Linux.

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
