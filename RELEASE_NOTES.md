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
  insect and fish books and the shops' CDs, part-time work and berry seedlings are open to visitors too. Two copies of the game on one PC find each other too. Details in
  docs/COOP.md.
- **Co-op: one town, and each other.** While connected, everyone plays in the same town: what one player changes in
  the host's town (its flags, its people's state) reaches the others within a second, over a channel of our own beside
  the game's link. And the players can talk to each other: face the other student and press A for a chat (it raises
  your friendship once a day; at a point you become friends, and both see it) or to ask how you're getting along. The
  friendship is kept beside each player's save.
- **Co-op: best friends, or a couple.** Casting Friendship or Declare love together (the game's own two-player
  incantations) now brings the two of you closer, as it would with a classmate. Once you're close enough, talking to
  the other student offers an invitation to Lovers Point at a time you pick; they answer Yes or No on their own
  screen. Be there together at that hour and a scene of your own plays on both screens: one of you speaks from the
  heart, the other chooses "Let's be a couple!", "Let's be best friends!" or "Can we wait a little?" (the players
  decide, not the game). Best friends and couples can hang out (a couple's moments are a little warmer), and there's
  a gentle "Go back to being friends" with a confirm. Both PCs keep the same record.
- **Co-op: give each other items.** Talk to the other student and choose "Give an item": your bag opens, pick
  something, and it goes into their bag ("You gave the Beige Wardrobe to Bea!" / "Bea gave you the Beige
  Wardrobe!"). It's an exchange: if their bag is full you keep it, and if anything goes wrong on the way it comes
  back to you; if the connection drops on the way, it's sorted out the next time you meet. Items the game won't let you give away stay yours, and the first
  gift of the day brings you a little closer.
- **Co-op: shopping as a guest.** Shopkeepers can be talked to across their counters while you're connected, so a
  guest can order and pick up CDs at Harrows, buy berry seedlings at Daisy's and take on part-time work at The
  Emporium, with their own money, into their own bag.
- **Co-op: connect at once.** Settings > Network > Connect automatically (As host / As guest): as soon as you're
  free to move after loading, the visit starts by itself, wherever you are: no walk to the Gateway, no menus (the
  game saves first, as the communications room does). A guest keeps searching until the host is there. Or press F9
  (Co-op: connect now) at any time. You both arrive in the host's dorm by the Gateway.
- **Co-op: through the night into the next day.** A visit lasts until one of you chooses Disconnect (Select): the
  evening, the night and midnight don't end it. The game can't turn the day while connected, so when the new day
  comes (around 3:30 in the morning, by season) the visit ends by itself: each of you saves, the day turns in each
  town, and you connect again by yourselves, the guests arriving in the host's new day.
- **Co-op: your own lessons, the host's story.** While connected each player picks and attends their own lesson
  (the lesson you chose, "done for today" and the after-class story stay yours; a guest arrives with its own), and
  the town's story (the week's mystery and its stages, the festivals, a classmate's transfer) moves on only on the
  host's console, so a guest can no longer advance or finish the host's mystery unseen.
- **Co-op: no toilet cleaning for a visit.** Closing the game while a visit ends, after a lost connection ("Unable to
  receive data") or while the visit reconnects for the new day no longer counts as turning off without saving. Turning
  off without saving outside a visit still does.
- **Co-op: the tatami room** off the dorm hall opens to visitors on its days.
- Fixed a crash when a Multi-Card Play session started (a loop in the game's code was translated as calls).
- **F12** saves a screenshot to `screenshots` in the data folder.
- Keyboard, mouse (the stylus) and gamepads, all rebindable; layouts: one screen, stacked, side by side, main + side,
  single screen with picture-in-picture, two windows (F10 cycles them).

Saves, settings and logs live in `%APPDATA%\MagiciansQuest-Recomp\MysteriousTimes`.
