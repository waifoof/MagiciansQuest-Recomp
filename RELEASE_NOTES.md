Magician's Quest: Mysterious Times, recompiled for PC. No game data is included: choose your own ROM (USA) on the
start screen and the game is built from it once.

**Getting started:** extract the zip somewhere you can write to and run `MagiciansQuest.exe`. Pick the **Enhanced**
preset for the recommended setup (one 16:9 picture, 60 fps, smoothed text and textures). F1 opens the settings.

**What's in it**

- **One screen (16:9):** the DS's two screens are two halves of one view, so they are joined into one wide picture.
  Where only the bottom screen has 3D (classrooms, shops, rooms) it fills the window, with the top screen in a corner
  (F3 or the right stick: small, large, hidden). Menus show both screens side by side. Empty far sides that the game
  never built are filled from the scene beside them (Settings > Video > Fill empty edges).
- **Zoom (widescreen):** where one screen fills the window (rooms, shops, classrooms), the camera comes in just
  enough that the picture is filled with what the game really draws, not empty sides (Settings > Video > Zoom).
- **60 fps 3D on both screens**, high resolution (up to 16x), widescreen (16:9 to 32:9) in every layout.
- **Smooth 2D:** text, menus and sprites upscaled with smooth diagonals instead of blocks.
- **Upscale textures:** the game's textures enlarged 4x with a pixel-art filter (crisp edges, smooth diagonals).
- **Texture packs:** Settings > Video > Dump textures saves the game's textures to `textures\dump` in the data
  folder; put edited or redrawn ones (1, 2 or 4 times the size, same names) in `textures\load`.
- **Walk speed** (only your character), **text speed**, **quick transitions**, **fast forward** (hold Tab).
- **Your PC's clock** is the game's: days, hours and seasons pass with real time.
- **Pause in background** (the game waits while you're in another window), and the mouse pointer hides while
  you play.
- **Co-op from a new game:** the game's own multiplayer (visiting each other's towns) over a LAN or the internet,
  from the first day. Settings > Network > Co-op between PCs: one player Hosts, the others Join (the host is found by
  itself on the same network; over the internet, type its address and forward UDP port 49251 to it). Start your
  games, go upstairs to the Gateway in the dorm, communications room, Multi-Card Play: the host picks "Come and play
  with me!", the others "Can we play together?". While connected, the first day's holds are lifted, so you can walk
  out into the host's town together right away, walk to the school (south past the shops, then up the slope to the
  striped gate). Walking into the school together starts the principal's welcome for both of you (the speech,
  the introduction to the class, the wand and hat), which ends the first day in each player's own save; the
  principal and teachers are there while you're connected, so you can take classes together. The café, the
  insect and fish books and the shops' CDs, part-time work and berry seedlings are open to visitors too. While connected the
  town also shows on the top screen. Two copies of the game on one PC find each other too. Details in
  docs/COOP.md.
- Fixed a crash when a Multi-Card Play session started (a loop in the game's code was translated as calls).
- **F12** saves a screenshot to `screenshots` in the data folder.
- Keyboard, mouse (the stylus) and gamepads, all rebindable; layouts: one screen, stacked, side by side, main + side,
  single screen with picture-in-picture, two windows (F10 cycles them).

Saves, settings and logs live in `%APPDATA%\MagiciansQuest-Recomp\MysteriousTimes`.
