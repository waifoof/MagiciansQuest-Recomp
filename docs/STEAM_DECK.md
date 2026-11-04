# Steam Deck (Linux) support

Goal: a player on a Steam Deck downloads one Linux file, sets the game up from her own ROM, plays at 1280x800 with
the Deck's controls (the touchscreen as the DS's touch screen), and plays co-op over the internet with a player on the
Windows build. This page is where that stands. Packaging in general: [PACKAGING.md](PACKAGING.md); co-op over the
internet: [COOP_INTERNET.md](COOP_INTERNET.md).

## Status (2026-11-03, phase 2)

| Piece | State |
|---|---|
| AppImage package | **Done.** `python3 tools/package.py linux` (Docker, Ubuntu 22.04: glibc 2.35) writes `dist/MagiciansQuest-Recomp-<version>-x86_64.AppImage`, about 175 MB. `--native` builds the same package on the machine itself (a test package, below). |
| Download | **Done (on the next push to main).** The `linux` job of `.github/workflows/windows-package.yml` puts the AppImage on the rolling `latest` pre-release next to the Windows zip: `https://github.com/waifoof/MagiciansQuest-Recomp/releases/download/latest/MagiciansQuest-Recomp-0.1.0-alpha-x86_64.AppImage`. Tagged releases (`release.yml`) already had a Linux job; it now makes the same file. |
| Setup from the ROM on the player's machine | **Done, tested** (below): the AppImage carries clang 22.1.8 and lld (with lld's libxml2/ICU/liblzma, which differ between distributions: SteamOS has libxml2.so.16) and clang's own headers; the setup builds the game module with them. |
| Data folder, portable mode | **Done.** `~/.local/share/MagiciansQuest-Recomp/MysteriousTimes` (SDL's pref path); `portable.txt` next to the `.AppImage` file keeps a `data` folder there. |
| Presenter | **Done.** OpenGL (the DXGI flip presenter is Windows only; Linux has always used GL). |
| Problem report, log | **Done.** Settings > About makes the report; the log is `logs/game.log` in the data folder. System information on Linux: the OS (`/etc/os-release`), CPU, memory, "Package: AppImage". |
| Open a URL / a folder | **Done** (`xdg-open`, `game/src/report.cpp`). Untested in Game Mode, where no browser may open: the report's folder is also shown as text. |
| File dialog (Select ROM) | **Done: our own file browser on Linux** (`game/src/ui/file_browser.cpp`), drawn by the UI, so it works in Game Mode with the controls only: D-pad / left stick to move, A opens a folder or picks the ROM, B goes up a folder (in the places list: closes), the triggers page through a long folder; the mouse and touch work too. It starts in the first of the home folder and the places below that holds a `.nds` file, with that file focused (so a ROM in Downloads or on the SD card is one press of A away), else in the home folder; later in the run, where the last file was picked; places on the left: Home, Downloads, Desktop, Documents, each SD card or USB drive (`/run/media/<card>`, `/run/media/<user>/<label>`, `/media/...`), the AppImage's folder ("Next to the program"), `/`; "System file dialog" opens SDL's (the desktop portal or zenity) as before. Only folders and `.nds` files are listed (hidden ones left out), sorted naturally. Windows keeps the system's dialog; `MQ_FILE_BROWSER=1` / `0` forces ours / the system's (tests: `MQ_FILE_BROWSER=1` shows ours to a `--ui-script`, which otherwise picks with `choose:`). |
| Crash handler | **Done.** A signal handler (SIGSEGV/SIGBUS/SIGILL/SIGFPE/SIGABRT, on an alternate stack on the main and the game's thread, so a stack overflow is reported too) writes `crashes/crash-<time>.txt` in the data folder (the working directory in test runs), as the Windows handler does: the signal and address (an abort: the runtime's last fatal message), the registers, a `backtrace()` (module+offset: `addr2line -e <module> <offset>`), and the executable mappings; then the signal ends the process as before (exit 139 / 134). Then, as on Windows, the crash dialog: a second process (`<program> --crash-dialog <report> <pid> <data folder>`, forked from the handler in a session of its own; from an AppImage the `.AppImage` file again, since the crashed one's mount goes away) with Restart game (the crashed command line, kept in `crash-<time>.cmdline`), Report issue (the zip and the filled-in GitHub issue), Show details, Close. On Linux SDL has EGL and no GLX, so the dialog asks for EGL. `MQ_CRASH_DIALOG=1` shows it in a test run, `0` never. Tested under Xvfb: `--crash-at-frame 300` -> report and dialog; Restart (Return) started the game again (its command line changed to `--abort-at-frame 300`), which aborted -> a second report and dialog; Escape closed it. |
| Co-op with the Windows build | **Same protocol** (checked below). A Linux host and a Linux guest, both from the AppImage, connect and play over internet mode at ~215 ms with 1 % loss (below). Not yet tried between two machines or against the Windows build. |
| 1280x800 layout | **Done.** The first start on a Steam Deck (`SteamDeck=1`) or a 16:10 display picks Main + side with a medium side screen: the touch screen 800x600, the top screen 480x360 beside it (and fullscreen on the Deck). Settings > Video > Side screen size: small / medium / large. Clicks / touches reach the touch screen through the layout (checked below). See "Screen layout". |
| Gamepad-only start screen | The start screen, Settings and the Welcome presets are controller-navigable (Dusklight's UI port). Audited at 1280x800 with the controls only (2026-11-03, `--ui-script` pad steps, below): fixed the co-op lobby and Controls opening with the focus on their one (invisible) tab, the start screen left with no focused button after the ROM's setup, the keypad's B (now deletes a letter), the setup message breaking inside words. Not tried under Gamescope yet. |
| Touchscreen | The program has no touch (finger) handling of its own: it relies on SDL turning touches into mouse clicks, which the program maps to the DS touch screen. Gamescope (Game Mode) gives touches to games as mouse input by default; to try on a Deck. The Docker build has XInput2 (needed for touch under X11 on the desktop). |
| Sound | SDL with PulseAudio and ALSA (Docker build). SteamOS has PipeWire with its PulseAudio server. |

## How the AppImage is made

`tools/package.py linux` builds the image `mq-linux-build` from `tools/docker/linux.Dockerfile` (Ubuntu 22.04, the
pinned LLVM 22.1.8 for the build and for the package's compiler, pinned appimagetool and type2 runtime), builds the
program with `-DFFCC_PACKAGE=ON` (libstdc++ and libgcc linked statically), runs the unit tests under Xvfb, refuses an
SDL configured without sound or XInput2, and assembles the AppDir:

```
AppRun                               runs usr/bin/MagiciansQuest
usr/bin/MagiciansQuest               the program
usr/bin/toolchain/bin/clang(-22)     what setup.cpp runs (it looks for toolchain/ next to the program)
usr/bin/toolchain/bin/ld.lld         a script: lld with toolchain/lib on its library path
usr/bin/toolchain/bin/lld
usr/bin/toolchain/lib/               the libraries lld loads that are not on every system (libxml2, ICU, liblzma)
usr/bin/toolchain/lib/clang/22/include   clang's own headers
usr/share/doc/MagiciansQuest-Recomp/ LICENSE, NOTICE.md, licenses/
magiciansquest.desktop, magiciansquest.png
```

Before writing the AppImage it checks that the packaged clang and ld.lld build a freestanding object and a shared
library with nothing else on the PATH (and that lld took its libraries from the package), scans the AppDir for game
data, and checks the newest glibc symbol version any file in it needs (2.35 at most from Docker).

`--native` does the same without Docker, on the machine itself (build folder `build/pkg-linux`, the LLVM archive,
appimagetool and the runtime downloaded once into `build/pkgcache` and checked against the pinned SHA-256). Its glibc
baseline is the machine's (refused above 2.39, the Steam Deck's), and its SDL has what the machine's `-dev` packages
give it: on a machine without them it prints "A TEST PACKAGE, not for release" with what it lacks (sound, touch...).
`--native --smoke <ROM>` sets the game up from the ROM into a new data folder with the package's own compiler (and
checks that it was that compiler), then boots it full screen at 1280x800 under Xvfb and saves `dist/smoke-linux.png`.

## Tests run (2026-11-03, on the Linux dev machine)

- **Build:** `package.py linux --native` from f5f3f5a (Ubuntu 24.04 host, no Docker registry access there, so the
  Docker path was not run; CI runs it). 173 MB; newest glibc needed 2.38 (the program). Toolchain libraries bundled:
  libxml2.so.2, libicuuc.so.74, libicudata.so.74, liblzma.so.5. A test package: this machine has no sound or XInput2
  headers.
- **Setup:** from the USA ROM into an empty data folder with the packaged clang
  (`setup: compiler: .../usr/bin/toolchain/bin/clang`): 12,337 functions recompiled and built in 150 s on 2 cores
  (with other work running).
- **Boot:** full screen on a 1280x800 Xvfb display (llvmpipe OpenGL): the start screen (Play, Settings, Quit, "ROM
  ready"), then the game. See "Screen layout".
- **Co-op** (`/home/claude/run/live/inetpair.sh` with the AppImage as the binary and the module its setup built): a
  Linux host (day1done.sav) and a Linux guest (second_day1.sav, another student) in internet mode with
  `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`: each copy started its stand-in from inside its own AppImage mount; connected
  by Connect now in 134 s from start, the guest in the host's dorm at 139 s, the stand-ins "in the session, playing
  together" at 207-236 ms; each player sees the other walk.
- **Same game:** the 80,000-frame regression (regs2) with the packaged program and the module its setup built
  (clang 22 at -O1): all 8 shots identical to the dev build's.

## Cross-play with the Windows build

What both builds share, checked in the source:

- The internet link (`runtime/src/lan_wireless.cpp`, the side channel and save transfer in `runtime/src/coop_net.cpp`)
  is the same code on both; the `#ifdef _WIN32` parts are only the socket API (WSAStartup, `int` vs `socklen_t`,
  `SO_RCVTIMEO` as a DWORD vs a timeval). The datagram header is `#pragma pack(1)` with fixed-width fields.
- The game's messages (`game/src/coop_*.cpp`) are built with `memcpy` of packed structs of fixed-width integers (no
  `long`, `size_t`, pointers, bit fields or `bool` in them), so the layout is the same under MSVC's and the Itanium
  ABI. Both are little-endian x86-64.
- Nothing random is computed on both sides and compared: the few `std::rand()`/distribution uses are one side's
  choice that is then sent (a fish epoch, a hall pick, the test-only loss/delay).
- The stand-in (the hidden copy each PC runs for the other player in internet mode) is started from the program's own
  path (`/proc/self/exe` on Linux; inside the AppImage's mount, which lives as long as the player's copy) with the
  same game module.
- Same port (UDP 49251) and the same "Host's address" join on both.

Windows-only in or around the net code (none of it used by Magician's Quest co-op):

- `runtime/src/instance_lan.cpp`: DS-internet (WFC/NAT) frames between copies on one PC; Linux returns "not
  available". FFCC's online play used it; MQ's co-op does not.
- The same-PC link (`coop_local.cpp`, `local_wireless.cpp`) has a POSIX version (shared memory, named semaphores):
  that is what the stand-ins use on Linux.

- The host's port opened on the router by UPnP / NAT-PMP (`runtime/src/portmap.cpp`, dec195f, after the AppImage
  tested here was built): the same code on both, only the socket API differs.

Still to check on real machines: a Windows host and a Deck guest over the real internet (the Windows PC as host is the
easier way round: its router opens the port by UPnP, or the host forwards UDP 49251), and that a Windows save
transferred to the Deck's stand-in (and back) loads.

## Screen layout at 1280x800

The Deck's screen is 16:10. The DS's two screens are 4:3 each.

| Layout | Size of each DS screen at 1280x800 | Note |
|---|---|---|
| Side by side (default) | 576x432, the pair 1152x432 (measured on the smoke test's start screen) | about half of the height unused (bars above and below) |
| Stacked | 533x400, the pair 533x800 | 58 % of the width unused |
| One screen (the 3D screen large, the other as a picture-in-picture) | 1067x800 | Best use of the screen; the other screen is small |
| Main + side, small side screen | main 985x738, side 295x221 | the main screen nearly full height; the top screen at 1.15x the DS |
| Main + side, medium (the Deck's first start) | main 800x600, side 480x360 | both readable: the top screen's text at ~1.9x |
| Main + side, large | main 704x528, side 576x432 | |

Main + side's side screen size (`display.side_size`: small, medium, large; Settings > Video > Side screen size): the
side screen takes at least 30% of the main one's width (small, as before), or 3/8 or 45% of the window's width.

The first start (no `settings.ini`, no `--layout` on the command line) on a Steam Deck (`SteamDeck=1`, which Steam
sets) or on a display whose desktop mode is 16:10 sets Main + side, the bottom (touch) screen as the main one, the
side screen medium and at the top, and on the Deck fullscreen (`main.cpp` `deck_defaults`; `MQ_DECK_LAYOUT=1` does it
on every start, for testing). The Welcome presets come after: Original keeps it; Enhanced picks One screen, which at
16:10 fills the screen with the joined 3D but shows menus side by side.

Checked at 1280x800 (Xvfb, `--resize-at-frame 50 1280x800`, the regression's inputs to frame 10,000, the dorm): the
measured rectangles above; an input-script `click 50,150` (window pixels) became `touch 16,16` and opened the bag on
the main screen; side size switched live with `--set-at-frame`.

## Controller-only audit at 1280x800 (2026-11-03)

Every screen driven by `--ui-script` gamepad steps only (`pad:south` = A, `pad:east` = B, the D-pad, the shoulders),
`SteamDeck=1`, a 1280x800 window: the start screen, the Welcome presets, the co-op lobby (Host, Join, the keypad: a
13-letter code typed, B, Done), Settings (all seven tabs with RB / LB, rows, B), Controls, the in-game menu strip (R +
Start, left / right, B), Quit's confirmation, and the first run from an empty data folder (Select ROM, the file browser,
the build, the start screen, the lobby). What was found and changed:

- **The co-op lobby and Controls** (windows of one tab) opened with the focus on the tab, which shows almost nothing:
  the first Down only reached the first button, and B went to the tab before it closed the window. A window of one tab
  now focuses its first control; Up at its top stays, B in its first pane closes it (`Window::single_tab`). The
  lobby's Host / Join views get the same after their rebuild.
- **The keypad** (Enter the code with the controller): B closed it and lost the letters typed; B now deletes the last
  letter, and closes only an empty keypad. The 13th letter moves the focus to Done. The line above the keys says "A
  types, B deletes".
- **After the first setup** the start screen had no focused button (the list itself kept the focus from while Select
  ROM was disabled): the first A did nothing. The start screen now gives the focus back to its first button.
- **The file browser** started in the home folder; a ROM in Downloads took four presses to reach. It now opens in the
  first of home, Downloads, Desktop, Documents, the SD cards and the program's folder that holds a `.nds` file, with
  that file focused: Select ROM, A, A.
- **"This takes about a minut / e"**: the build window's sentence broke inside words (the file-path style breaks
  anywhere, for long paths); it breaks between words now.
- **The in-game hint** said "R + Start" whatever the pad; it now names the connected pad's buttons: "RB + Menu" on an
  Xbox pad and the Deck (Steam Input shows an Xbox pad), "R1 + Options" on a PlayStation pad.

Checked and fine: Settings' tabs and rows, B out of every pane, the Quit confirmation (Cancel focused), the in-game menu
strip, the lobby's Host view (code, router, Partner, Play disabled until a partner), the keypad's movement (Down from
the last row reaches Done, Up from Done the keys). Seen, not changed: the Settings window opens with the focus on its
tab bar (several tabs: RB / LB or Down) and its right pane empty until a row is focused; the codes' zero is an old-style
figure in the UI font (looks like a small o; the codes have no O, so a typed O is the only risk); the default pad
layout is the DS's by position (the right face button is the DS's A: on a Deck or Xbox pad that is the button labelled
B, while the menus confirm with the bottom one, A).

Test notes: `shot:` saves a frame a few frames later, so put a `wait:` after it before the next press; under load
llvmpipe runs the UI at a few frames a second, so a pad press (down, up a frame later) can outlast the 0.32 s key
repeat and count twice: compare with the UI's focus log before calling a jump a bug.

## Fast forward and screenshot on the pad

Every pad button has a DS button or the stylus (both triggers touch), so fast forward and screenshot are **pairs**
with View (the Deck's left menu button, the DS's Select): hold **View** and press **RB** for fast forward (keep both
held), **View + LB** for a screenshot (2026-11-04). Not R + Start (RB + Menu), which opens the in-game menu.

- While View is held for a pair the game gets neither button: View's own Select waits until View is let go and is
  then pressed (a tap), and only if no pair was used meanwhile; RB / LB are not the DS's R / L while View is held.
  (Select asks "Disconnect?" in a co-op visit: a pair must not reach it.) View alone still works as Select, on its
  release. Press View first; RB first is the DS's R until View comes.
- **Remappable:** Settings > Input > Configure Inputs, the Gamepad page: for the rows from *Show other screen* down a
  pair is chosen by holding one button and pressing another ("View + ..." shows while the first is held); a single
  button counts when it is let go alone. Saved as `controls.fast_forward.pad = back+rightshoulder`. A settings file
  from before (no `controls.pad_pairs` line) gets the two pairs where it had no pad input for them.
- Fullscreen (F11) and Connect now (F9) still have no pad input by default: bind a pair, or give a back button
  (L4/R4/L5/R5) the key in the game's Steam Input layout. Steam's own screenshot (Steam + R1) works too.
- Fast forward does nothing while playing together (both games keep the same speed): the pair says so once.

Checked (2026-11-04, a solo copy with `MQ_VPAD`, docs/TESTING.md): View + RB held 95 frames: fast forward on, no DS
button; let go: no Select. View + LB: a screenshot file, no DS button. View alone: Select for 6 frames at its release.
RB alone: R. RB then View: R until View, then fast forward, R again once View is let go, no Select. The settings
test (`tests/test_settings.cpp`, `test_pad_pairs`): `back+rightshoulder` read and written, "View + RB" / "Share + L1"
labels, an old file given the pairs, a new file's cleared one kept cleared.

## What is left for the Deck (estimates)

1. **First CI run** of the Linux job (Docker path) after the merge to main: check the AppImage it publishes (size, the
   "lacks" check passed, ~30 min of CI). If the Docker image fails to build: fix (~1 h).
2. ~~Game Mode ROM setup~~ (done: the file browser). Could still add a ROM search (look for a file of the right size
   and SHA-1 in the places and offer "Use this ROM"), ~2 h.
3. ~~Linux crash handler~~ (done).
4. ~~Deck layout default and touch~~ (done; on a real Deck: check the touchscreen reaches the main screen under
   Gamescope).
5. **On a real Deck** (the player, ~1 h): Add to Steam (Desktop Mode: right-click the AppImage, *Add to Steam*), Game
   Mode start, the Welcome presets and the start screen with the controls only, the touchscreen in the game, sound,
   frame rate at 60, a co-op session with the Windows PC. A Steam Input template is not needed: the Deck's controls
   reach the program as an Xbox-style gamepad.
6. **Internet without port forwarding** where the router has no UPnP / NAT-PMP (open): the host forwards UDP 49251 by hand.
