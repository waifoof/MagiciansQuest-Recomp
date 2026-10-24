# Co-op

Where co-op stands, what the game itself offers, and the road from there to playing the whole game together.

## Playing together (from a new game)

1. **Connect the PCs** (each player: Settings > Network > *Co-op between PCs*). One picks **Host**, the others
   **Join**. On the same network, Join finds the host by itself; over the internet, type the host's public address
   in *Host's address* and have the host forward UDP port 49251 to their PC. Restart the game after changing it. (Two
   copies of the game on one PC need none of this.)
2. **Start your games**: a new game each (or a save each). The first day is fine.
3. In the dorm, go **upstairs to the Gateway of the Beginning and End** (the big golden door) and touch it, then
   choose **communications room**.
4. The host picks **Come and play with me!**, then **Multi-Card Play**, and waits. Each guest picks **Can we play
   together?**, then **Multi-Card Play**, then the host's town.
5. You're in the host's dorm together. While connected, the first day's holds are lifted for everyone (the warden
   is back at his counter instead of waiting at the way out), so you can walk out of the dorm (the bottom of the
   downstairs hall) into town together straight away.
6. **To the school**: from the square in front of the dorm, go down (south) to the shops, then west and up the slope
   to the lily pads and the striped gate; the street beyond leads to the school's red doors (face them and press A).
   On the DS a new student is walked there by the warden; while connected you walk it yourselves.
7. **Class**: upstairs, Principal Sol waits by the classroom door (during a visit the game leaves the school's staff
   out; while connected they are there, `mq_coop_npc`). Each of you talks to him and takes the lesson; verified with
   two day-one games: both took the first lesson and stood in the classroom together afterwards, still connected.
   (Each console runs its own student's lesson: during it you don't see each other.)

## What the game has

Magician's Quest's multiplayer is visiting: up to four students meet in one town, over Multi-Card Play (DSes nearby)
or the Nintendo Wi-Fi Connection.

- It starts at the **Gateway of the Beginning and End**, upstairs in the dorm: touching it asks "Where would you like
  to go?" and offers the **communications room**. There, a menu offers *Can we play together?* (visit another town),
  *Come and play with me!* (invite others to yours), *friend code* and *Exit*, then *Multi-Card Play* (nearby) or
  *Nintendo WFC* (far away).
- A classmate with you in the communications room can be sent along as a *visiting student* (study abroad), for a few
  days.
- On the DS, the gateway answers "You cannot use it at this time" until the first lesson is over: its script (overlay
  28, command 0x38) tests variable 509, which a new game sets and the first lesson clears. On the first day the
  warden also waits at the dorm's way out and stops a new student ("Have you already seen enough of the dorms?") to
  walk them to school. **Here the gateway opens from the first day, and while a session is on, variable 509 reads as
  0 for everyone** (as after the first lesson; nothing is written, so the first day is back when the session ends):
  the warden is at his counter and the way out is open (`mq_coop_gate` and `mq_coop_var` in
  `game/src/cfie_hooks.cpp`; the same rule on every console, which run in lockstep). Verified: two copies of the
  game, both on day one, went through the gateway to the communications room, chose Multi-Card Play, one walked
  into the other's dorm, and both walked out into town together, each seeing the other.
- The purple door off the downstairs hall ("The door is locked" on the first day) is not the way out: it leads to a
  small tatami room.
- While visiting, some things are refused: buying berry seedlings, some shop services ("you can't use that service
  while visiting another town"), some incantations ("cannot be used during wireless communications"), some items.

### Notes for the next steps

- **"In a session"** is one pointer: `*(*0x020ACA58 + 0xA28)` (the game object's wireless session; null when not
  connected). About 200 places read it (main, the town overlay 1, overlays 0, 3, 7, 9, 16, 24, 28); it is also how
  the game reaches the session to keep the consoles in step, so it cannot be faked as null. The refusals are lifted
  one at a time, where they are decided.
- Event scripts ask the code with command 0x53 (`53 08 <query u16> <arg u32>`), the answer in variable 10000, then test
  it with 0x38. Query 0xB5 (overlay 28: 0x0211A16C) is "may this incantation be used": 0 in a session, so the script
  shows "This incantation cannot be used during wireless communications" (script `event/345.bin`, block 1). Goto
  targets in 0x38 (`+12`) are command indexes in the block.
- On the first day the warden's escort takes the new student from the dorm hall straight to the school's square
  (position 44, 41.8; the school's door at 44, 29.5), a warp. On foot: the dorm's square (x 71–84, z 46–60, height
  4.8) opens to the south (the shops), and the lower town climbs to the school's square (height 11.7) at x 43,
  z 52 → 40. Verified in a session on day one: both students walked to the school and went in (entrance hall at
  12, 27).
- **The first day's welcome** (the principal's speech, the wand and hat; at its end variable 509 is cleared, which
  ends the first day) is a story event: it starts on entering the school when the warden's escort has run
  (variable 559 = 1, set in the dorm's intro). During a session the game uses another set of events and NPC
  triggers (`/npc/NpcTrgDM.bin` instead of `NpcTrg.bin`, chosen in overlay 1 0x020E2ABC; event files 009/010
  instead of 000/001), which has no story: no escort, no welcome. So today a first day spent together does not end
  the first day; each student's welcome still happens alone afterwards. (What selects that set during a session is
  not found yet: the byte 0x020AD7FF that 0x020E2ABC tests is 0 then.)
- Event scripts mostly write the local student's own record (player -1), and that record is kept: verified with a
  guest who talked to the post office's clerk in the host's town (a flag in the guest's own record changed), then
  disconnected: the flag was unchanged after "Returning to your own town" and the save that follows.
- The player's position is at 0x020B8B7C (x, y, z, 20.12 fixed point; per area).
- In a session the top screen shows a 2D pattern instead of the town (the 3D is drawn on one screen only); on the DS
  that keeps the frame rate up while the radio runs.
- Test drivers (not in the repo): `MQ_TEXT_LOG` (each new text box), `MQ_POS_FILE` (the position), `MQ_SCRIPT_TRACE`
  (event commands), `MQ_RAM_DUMP`, `MQ_COOP_SOLO` (the session's rules without a session).

The game is lockstep (`docs/ARCHITECTURE.md`): every console runs the same simulation from the players' inputs, so
whatever the host's town does, every guest sees.

### What a session refuses, and where (catalog)

Four mechanisms decide it:

- **A, menu text:** a tag `$016'A'B` in a menu option jumps to message B instead of A while in a session (main menu
  parser 0x02038938, the switch at 0x02038A9A). Used by the CD shop (buy/pick up, 5150), the general store's part-time
  work (5353), the flower shop's berry seedlings (5780) and the post office's Download (8300).
- **B, flag byte 212** (script variable 10212): `session[0x6912] + 1` while connected (set ov001 0x020F2300, cleared at
  the session's end, ov001 0x020EE0E6 / 0x020EE142 / 0x020EE1A6). Scripts test it with 0x38: the school bell (8614),
  registering in the insect and fish books (8634, 8649), the locker's explanation (8673), the storage chest (999:1021),
  the café (32:6230), one room door (999:5306), and the summon and transform incantations (3120: World Tree, Phantasm,
  Werewolf, Homunculus, Familiar, Golem, Charm and one more).
- **C, query 0x22:** in a session (variable 10020) and the console's slot (variable 10000), ov028 0x021152D0: the
  communications room (no second session), shopkeepers' walks.
- **D, query 0xB5** (ov028 0x0211A16C): the incantation's result id is flag byte 199. Connect doors, Sweet dreams, Make
  peace, Sit by me, Honor student and Mushroom are always refused (3114-3119); Rainbow, Flower rain and Rainmaking ask
  the other console (packet type 0x45) and Friendship and Declare love also wait for the target's state: these are the
  game's own two-player incantations.
- The bank (deposit, withdraw, safe deposit) uses another menu in a session (5808 for 5807; selector not found).

Script variables 10000+n are flag bytes n (`*(*0x020B8AAC + 0x14) + 0x2254`).

## What is built

- **Two copies on one PC** find each other (shared memory; Windows, and now Linux).
- **Two or more PCs** (`include/ntr/lan.hpp`, `runtime/src/lan_wireless.cpp`): the game's Multi-Card Play frames as UDP
  datagrams. One PC hosts (it hands each frame on to the others), the others join it, found by themselves on the same
  network or by address: Settings > Network >
  *Co-op between PCs* (Off, Host, Join), *Host's address*, *Co-op port* (49251 by default), or `--lan-host [port]` /
  `--lan-join <address[:port]|auto>`. Over the internet the host forwards that UDP port. The window's title shows who is
  connected. Each PC's console gets a MAC address of its own.
- **Far away** also works the game's own way, through the Wi-Fi Connection revival servers (Settings > Network).
- `tests/test_lan`: a host and two joining PCs over loopback through the calls melonDS makes (commands, replies,
  frames handed on, a PC leaving).



## The road to the whole game together

What was asked: start together, load into the world together, play the whole game side by side, and romance each
other. The game was not made for this: its story, classes and events belong to the one student of a save, and a
visit is a short trip to someone else's town. Each step below changes more of the game, so each is its own milestone.

1. **Visits that work everywhere, from the first day** (here now): host/join between PCs, the game's own visit, and
   out into the host's town together on day one.
2. **Stay together**: a visit that lasts — the guest follows the host through doors, into buildings, the school and
   events instead of the session ending, and the "not while visiting" refusals lifted where the game copes (hooks on
   those checks). Needs: where the game ends or limits a session, found in code.
3. **Start together**: both players' students in one town from the first day. The save holds one student; a second
   needs a save layout of our own beside the game's (the guest's student, items, money, friendships), written back to
   the guest's own save after each session so neither loses progress.
4. **The story together**: classes, events and cutscenes are scripted for one student. With both present, the second
   watches and joins where the scripts allow (they are data, `talkfigs`/`script`), and gets the same rewards.
5. **Romance between players**: the game's dating is between a student and classmates, with its own dialogue and
   events. Between two players it would be new content: new lines, new events, and their state in our save layout.
   Possible once 3 and 4 exist, as an addition rather than something unlocked.

Each milestone keeps the lockstep: everything added must run the same on every PC from the same inputs.
