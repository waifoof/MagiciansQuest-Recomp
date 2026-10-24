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
7. **The welcome**: walking into the school together starts the principal's welcome for both of you (his greeting
   in the hall, then the classroom: the introduction to the class, the wand and hat). It ends the first day in each
   player's own save.
8. **Class**: upstairs, Principal Sol waits by the classroom door (during a visit the game leaves the school's staff
   out; while connected they are there, `mq_coop_npc`). Each of you talks to him and takes the lesson; verified with
   two day-one games: both took the first lesson and stood in the classroom together afterwards, still connected.
   (Each console runs its own student's lesson: during it you don't see each other.)
9. **Talk to each other**: walk up to the other student, face them and press A (with no one else in front of you).
   They greet you and you choose: *Chat* (four topics in turn; the first chat of each day brings you closer), *How
   are we getting along?* (acquaintances, friends, best friends or a couple) or *Never mind*. On the other screen
   their student turns to you ("<name> came over to talk with you"), and is told about the chat. Each PC keeps the
   pair's record beside its save (`<save>.coop`, e.g. `cfie.coop`); see docs/COOP_RELATIONSHIPS.md.

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
  `game/src/cfie_hooks.cpp`; the same rule on every console). Verified: two copies of the
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
- **The first day's welcome** (event 001: block 6 the principal's greeting, block 7 the class introduction, the
  wand and hat, and variable 509 cleared, which ends the first day) starts from a start record in
  `start/mc_ss.bin` (block 6, record 0: variable 509 = 1 and the step byte 10233 = 2, kind 4) on entering the
  school. On the DS the step byte becomes 2 when the warden's escort (block 5) has run; a session has no escort.
  Three things make it run in co-op: the story records (kind 4) are allowed (`mq_coop_npc`); a step still at 1 reads
  as 2 (`mq_coop_var`, variable 10233); and the record check (ov001 0x020F4214, the record in r4) gives the true
  first-day value to event 001's blocks 6 and 7 only (other first-day records, the dorm's scenes, would start in
  the middle of connecting). Verified with two day-one games connected: both walked into the school, saw the whole
  welcome together, ended up in the classroom together, still connected; the guest then disconnected and in their
  own town the warden went on to the scholarship (the day after the welcome), so the end of the first day was kept.
- NPC triggers in a session: overlay 1 0x020E2ABC picks `NpcTrgOL.bin` (the online set, no story) when its r1 is set
  (field+0x3EAF set and field+0x3EB4 != 16); `mq_coop_triggers` makes it pick `NpcTrg.bin` as when alone.
- Event scripts mostly write the local student's own record (player -1), and that record is kept: verified with a
  guest who talked to the post office's clerk in the host's town (a flag in the guest's own record changed), then
  disconnected: the flag was unchanged after "Returning to your own town" and the save that follows.
- The player's position is at 0x020B8B7C (x, y, z, 20.12 fixed point; per area).
- In a session the DS shows a 2D pattern on the top screen instead of the town (the 3D on one screen only, to keep
  the frame rate up while the radio runs). Here the field's screen setup (ov001 0x020C6F1C) is asked for both screens
  in a session too (`mq_dual_in_session`): the town shows on the top screen while connected (verified outdoors and
  in the school's hall).
- Test drivers (not in the repo): `MQ_TEXT_LOG` (each new text box), `MQ_POS_FILE` (the position), `MQ_SCRIPT_TRACE`
  (event commands), `MQ_RAM_DUMP`, `MQ_COOP_SOLO` (the session's rules without a session), `MQ_WARP_FILE` (write
  "map x z" or "e <arrival point>" to the file and the player is sent there; e.g. `e 235` outside the café, `e 243`
  outside the books building, `e 233` outside Harrows), `MQ_EVENT_FILE` (write an event number, file * 100 + block,
  and it is queued; e.g. 90001, the demo of our own content, docs/COOP_RELATIONSHIPS.md).

**How the consoles keep together** (verified in a session, gdb on the recompiled functions): not by lockstep. FFCC's
consoles all run one simulation from everyone's inputs (`docs/ARCHITECTURE.md`); Magician's Quest's do not. Each
console runs its own student and its own copy of the town, and sends its student's state in 16-byte packets (main
0x0202C4CC: `[0]` type, `[1]` the sender's session slot, filled by ov001 0x020F11E4; one outgoing slot at
session+0x4CB0, which only packets of types 0x0E-0x12 may overwrite (movement [I]: 0x0F goes every frame with the
position; 0x13 a change of action, 0x1E seen when a student starts talking to an NPC, 0x42 a telegram, 0x45 an
incantation). The
other consoles put each player's packets in that player's object (a queue of 4 at +0xE4, read by its update, main
0x02031404, a jump table on types 0x0F-0x32) and keep the last two at the session's entry for the slot
(session+0x4A08 + slot * 0x50). Scripts, menus and text run on the console whose student is in them; the others
see only what the packets show (the other student standing, turning, talking). A rule of ours that is the same on
every console (the hooks above) is the same because it reads the same data, not because the consoles step together.

**What a session shares** (measured: two copies, clocks 8.5 h apart, RAM dumps and a gdb packet log):
- **At connect, in bulk** (main 0x02050140, session+0x4CC0): the host's whole town save block, 0x993C bytes from
  0x020AF020 (town-time object +0x2C8, global variable store +0x2D8, ground-object grid +0x438, classmate records,
  shops), received by the guest at ov001 0x020EF552; each student's record, 0x1858 bytes (local 0x020AD824; the others'
  copies at session+0xE0 + slot * 0x1858); and the host's date/time. After ~10 minutes idle the two blocks were still
  byte-identical except checksum and time offset. Every later write stays on the console that made it.
- **Time is synced by the game**: the guest stores the difference between the host's time and its own RTC in the
  town-time object (block+0x2C8: date, time, day offset +8, second offset +0xC; main 0x02011F84 at ov001 0x020EF0E2)
  and runs on host time to the second; lighting follows it.
- **Per frame**: one 16-byte packet per console, and movement (0x0F) fills it every frame; nothing else is sent
  unless something happens. Types 0x33-0x47 (putting objects on the ground, taking them, reservations, telegrams,
  incantations) are **arbitrated by the host** (ov001 0x020F0E50 / 0x020F0C4C against its own town grid) and
  re-broadcast.
- **Not shared**: the RNG (Mersenne Twister at *0x020AD6AC, seeded at boot from the RTC's seconds of the day; ~200 call
  sites in the NPC code of ov001 and ov027), so RNG-driven classmate behaviour and the cosmetic particles differ; the
  time-driven town NPCs agree.

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

**Lifted in co-op** (the local student's own business): the café, registering in the insect and fish books, the
locker's explanation (`mq_coop_gate`: the flag byte 212 test before each refusal is made one that never holds), and the
shops' CD, part-time work and berry seedling options (`mq_menu_session`: the menu keeps message A). Verified while
connected: the café (Marcy introduces herself instead of "we're not open yet") and the insect book ("If you're looking
to register..." instead of "you can't use that while communicating"). Still refused: the bell, the storage chest, the
locked room, the summons and transforms, the bank's own session menu, Download, a second session.

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

Each milestone keeps the consoles together the game's way: what a student does is decided on their own console, and
anything another console must know or show travels in a packet (as the players' talk does, below).
