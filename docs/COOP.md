# Co-op

Where co-op stands, what the game itself offers, and the road from there to playing the whole game together.

## Playing together (from a new game)

1. **Connect the PCs** (each player: Settings > Network > *Co-op between PCs*). One picks **Host**, the others
   **Join**. On the same network, Join finds the host by itself; over the internet, type the host's public address
   in *Host's address* and have the host forward UDP port 49251 to their PC. Restart the game after changing it. (Two
   copies of the game on one PC need none of this.)
2. **Start your games**: a new game each (or a save each). The first day is fine.
3. **Connect at once** (the quick way): Settings > Network > *Connect automatically*: the host picks **As host**,
   each guest **As guest**. As soon as you're free to move after loading (after the warden's welcome on a new
   game), wherever you are, the visit starts by itself: no walk to the Gateway, no menus to answer (the
   communications room's messages flash by while it connects; it saves your game first, as the room does). A guest
   keeps searching until the host's town is there (a few minutes at most). Or press **F9** (*Co-op: connect now*,
   rebindable in Controls) at any time: it connects as the role of *Connect automatically*, else of *Co-op between
   PCs*, else as host; F9 again while it waits calls it off. Both of you arrive upstairs in the host's dorm, by the
   Gateway, as from the communications room.
4. **Or the game's own way**: in the dorm, go **upstairs to the Gateway of the Beginning and End** (the big golden
   door) and touch it, then choose **communications room**. The host picks **Come and play with me!**, then
   **Multi-Card Play**, and waits. Each guest picks **Can we play together?**, then **Multi-Card Play**, then the
   host's town.
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
10. **Closer**: cast Friendship or Declare love together (both of you at once, as the game asks in a session): it
   raises your friendship or affection by the game's own amount. Once you're good friends (and for a couple, with
   some affection too), the next talk offers an invitation to **Lovers Point** at a time you choose (10 a.m. to
   8 p.m.); the other player answers on their screen. Be there together within that hour and your scene plays on both
   screens; the one who was asked chooses a couple, best friends or "wait a little". Best friends and couples get
   *Hang out* in the talk menu, and *Go back to being friends* (with a confirm).

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

### Connecting at once (how Connect now works)

The Gateway (event 250) warps the student to the communications room (map 16, `warp_map 16 (640, 0, 1088)`), whose
start record (`start/mc_ss.bin` block 16) runs event 271 block 1: it places the room, then asks **query 0x2A** (ov028
0x021158AC -> ov001 0x020F224C). That one query runs the whole room: ov001 **0x020F17EC** is a loop (one frame per
pass, in the script's task) that shows system messages of bank 999 through main 0x0201DD10 and acts on the message
the text box stops at (main 0x02017C38: the box's +0x1470 + 100 x +0x146C) and on its menu's answer (the script
context's menu at +0x1CE4: main 0x02039664 "decided", 0x02039420 the option, the cursor at +0x294):

| message | what | answer |
|---|---|---|
| 20, 22 | "Connecting...", "What would you like to do?" | |
| 40 | Can we play together? / Come and play with me! / friend code / Exit (the role: stored in fields 448 and 262) | 0 guest, 1 host |
| 41, 42 | Multi-Card Play or Nintendo WFC | 0 |
| 43, 45 | "Starting Multi-Card Play with nearby users..." Yes / No; Yes starts it (ov023 0x02111114 (role): saves, 46 "Saving.", makes the session object) | 0 |
| 47, 48 | host: "Preparing connection...", then "Preparation of connection complete." once the session is up (ov001 0x020C9AA8) | A: the loop ends, returns 1 |
| 49, 50 | guest: "Searching for a town to visit...", then the towns found as a menu (session methods +0x54 count, +0x58 each) | 0 (the first town) |
| 51-53 | "Unable to find a town to visit." "Search again?" | Yes (up to 30 times) |
| 54, 55 | "Connect to <town> from <school>?" Yes connects (ov001 0x020EFDA4), then 47, 48 | 0 |
| 23, 24, 57-59 | canceled / "anything else?" (back to 40) | (40 a second time: Exit) |

When it returns 1 the query sets the session's flags up (flag byte 212 = slot + 1 and others, 0x020F224C) and 271
warps to the dorm by the Gateway (`warp_map 14 (1920, 768, 128)`): the field loads again as a session field (the
guest's in the host's town; the host's own with the session's rules), and the host sees "Someone new has arrived."

**Connect now** (`game/src/coop_connect.cpp`): our event 90005 (`game/src/coop_content.cpp`) is 271's own steps
without the room: query 0x2A where the student stands, then the same warp. While the room's loop runs, its
questions are answered as a player would, with the game's own pad: `mq_coop_pad` (main 0x02003628, right after
0x020036D8 has read the DS's buttons into the pad at `*0x020ACAA4`: +4 held, +8 pressed, +0xA repeated) adds Down/Up
until the menu's cursor is on the option above, then A, a press every 4 frames, and A for every other message. The
event marks where the loop starts and ends with two tests no variable passes (`if t0 == 0x4D510001/2`), seen by
the hook on command 0x38 (`mq_coop_gate`). It is queued (main 0x0201DAF0, as the town's triggers) once the student
is free to move: the field up with its controlled actor (`*0x020FE264`), no event running (script context +8) or
being started (+0x23BC, the start kind: the dorm's first-day intro starts from a start record some 30 frames after
the student appears) for 90 frames. Everything on the way (the save, the session, the warp, the arrival) is the
game's own; without a request nothing of it runs (the pad hook returns at once).

Started by Settings > Network > *Connect automatically* (Off / As host / As guest; `online.coop_auto` off / host /
join: once per start of the game), the bindable action *Co-op: connect now* (F9), or `MQ_COOP_AUTO=host|join`
(tests; `MQ_COOP_CONNECT_FILE=<path>`: "host" or "join" written there asks as the hotkey does; `MQ_COOP_SEARCHES=<n>`
fewer searches). `MQ_COOP_CONNECT_LOG` logs each message it answers and the menu's cursor.

Measured with the harness (two day-one copies on one PC, shared memory link, software renderer, unpaced; the
machine shared with other runs): the menu flow (`go.sh`: intro, walk to the Gateway, menus) took 925 s from start to
connected (731 s of it to reach the Gateway, the menus 194 s; about 4 min on a quiet machine); Connect now
(`/home/claude/run/live/fastgo.sh`) 61-68 s from start to connected and 64-71 s to both standing in the host's dorm,
including the warden's intro (the room's part: about 20 s). Also verified: a guest started 9 searches before the
host kept searching until the host's town appeared, then connected; a guest alone gave up after its searches
(No, then Exit) and stayed where it stood; a host that asked from the school (after the principal's welcome)
connected and arrived in its dorm, and a guest then joined; both walked out of the dorm into the host's town together.

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
- Test runs side by side: `NTR_LINK_NAME=<word>` gives a set of copies on one PC a shared-memory link of their own.
- Test drivers (not in the repo): `MQ_TEXT_LOG` (each new text box), `MQ_POS_FILE` (the position), `MQ_SCRIPT_TRACE`
  (event commands), `MQ_RAM_DUMP`, `MQ_COOP_SOLO` (the session's rules without a session), `MQ_WARP_FILE` (write
  "map x z" or "e <arrival point>" to the file and the player is sent there; e.g. `e 235` outside the café, `e 243`
  outside the books building, `e 233` outside Harrows), `MQ_EVENT_FILE` (write an event number, file * 100 + block,
  and it is queued; e.g. 90001, the demo of our own content, docs/COOP_RELATIONSHIPS.md), `MQ_COOP_CAST_FILE` (write
  30 or 32: Friendship or Declare love is cast with the other student here, docs/COOP_RELATIONSHIPS.md; e.g.
  `49 6.2 3` to `MQ_WARP_FILE` is Lovers Point).
- Lovers Point is area (map) 49, the field's byte +0x3EB4 (the area shown). The town's own clock, which the start
  records' time windows test (ov001 0x020F44E0: hour * 100 + minute against bytes +0x24/+0x26 of an 80-byte record), is the town
  block's +0x2CC (hour) and +0x2CD (minute): host time on every console of a session.

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
- **Not shared**: the RNG (Mersenne Twister at *0x020AD6AC, seeded at boot from the RTC's seconds of the day). Most of
  its callers are not the classmates: of ov001's ~130 calls, 82 are the insects (manager field+0xA74C, 17 behaviour
  classes), 25 weather and the sky, 16 fish, 6 mushrooms; ov027's 73 are the day's map items and the classmates'
  day roll. Only four rolls sit inside NPC commands ("The town's people", below). So the insects, fish and cosmetic
  particles differ between consoles; the time-driven town NPCs agree.

### One town: the town kept in step (built)

The game gives a guest a copy of the host's town once, at connect, and each console writes only its own copy after
that. Now the host's town is the town of everyone in the session: `game/src/coop_sync.cpp`, over a side channel of
ours (below), from the end of each frame (`mq_frame_end`), only in a co-op session and only once the console stands
in the host's town (the field loaded in the session, field+0x3EAF; the guest's block is the host's by then).

- **The town block** (0x993C bytes at 0x020AF020). About once a second (60 frames) the host compares its block with
  what it last sent and sends the bytes that changed (runs of offset, length, bytes); a guest writes them into its
  copy and fixes the block's checksum (main 0x02011D9C, as the game does after receiving it). When a console joins
  (a hello with its session slot) the host sends it the whole block, which also corrects what changed between the
  game's transfer and then (in the test, the 20 bytes of the guest's own weather roll at +0x3F4, made as it
  connected).
- **A guest's own changes** go to the host as requests: the guest compares its copy with what it had from the host
  every frame (before the host's bytes are written in), and the bytes of the global variable store (+0x2D8,
  0x160 bytes) that its student's own actions wrote go to the host, which writes them into its block; the host's
  next comparison sends them back to everyone, with the number of the last request it wrote for each guest. Until
  then the guest keeps its own value for those bytes. Which writes are the student's: the store write main
  0x0201E070 (hooked before, `mq_var_store`) on the block's store, called by anything but a roll; a write through
  the variable setter main 0x0201D9F8 (`mq_coop_set`) counts as its caller's. The index decides the byte as the
  store write's code does (index < 0x100 a byte at +index, < 0x200 a bit at +0x140 + (index - 0x100) / 8,
  < 0x240 a byte at +(index - 0x100), < 0xA40 a bit at +(index - 0x240) / 8).
- **Not sent: the guest's own rolls**, which the host makes too (docs: the NPC research, `NPCSYNC.md`): the
  classroom's picks of classmates (ov003, store writes returning to 0x021061B2 / 0x021061E6 / 0x021066DE: variables
  251/252, 236-239), the day's weather (callers in ov001 0x020CFBC0-0x020CFE30: store indexes 540-559) and anything
  the guest's game changes outside the store by itself (relation drift on each scene load, ov001 0x020F5F08; the
  day's roll, ov027). Those stay on the guest as its own until the host's values arrive (the classroom's picks are
  used as soon as they are rolled: a guest who loads the classroom before the host may see other classmates in its
  lesson). When the guest's town date (+0x2C8) changes (its own day change) nothing it changes is sent for 120 frames,
  then it asks the host for the whole block again.
- **Each console's own bytes** are never sent: the checksum (+0, 2 bytes), the town time object (+0x2C8, 16 bytes:
  the date and time each console stamps from the shared time, and the guest's day and second offsets to its own
  clock), and the classmates' values toward the 4 player slots (+0x60..+0x6B of the 13 records at +0x6468, 0xA4
  each): on a guest, slot n is the visiting student's slot in its own town, not the host's student with that number.
- **Students' records**: each console sends the bytes of its own student's record (0x020AD824, +2..+0x17E3, the part
  the checksum covers) that changed, every 60 frames (and the whole on a hello); the others write them into their
  copy (the session's entry for that slot, +0x18) and fix its checksum.
- Test drivers: `MQ_COOP_POKE_FILE=<path>` (lines `<hex address>=<value>[/<bytes>]`, or `var <n>=<value>` through the
  game's setter, written at the end of the frame the file appears), `MQ_COOP_NO_SYNC` (the game's own visit),
  `MQ_COOP_SYNC_ALL` (a guest sends every change, rolls too).

**Verified** (two day-one copies, host clock 10:00, guest 18:30, Connect now; both over the same-PC link and over
UDP between "PCs" on loopback): the hello and the whole block (39054 bytes; 20 bytes of the guest's weather
corrected); the guest's `var 1200=42` reached the host as request 1 and came back in its next bytes; the host's
`var 1201=7` and a byte written into its store reached the guest within a second; a byte poked into the guest's
block outside an action stayed the guest's ("changed by itself, kept here"). Over about three minutes of both
students walking in and out of the dorm and about town (pressing A and B now and then), RAM dumps of the two blocks
taken together differed only in the checksum, the guest's time offset, the seconds byte of the town time stamp (the
dumps a moment apart) and that poked byte. Outside a session nothing runs (each entry tests the session first).

Open:
- Shops, the bank and other town state written outside the variable store by the guest's own actions are not sent
  (they cannot be told from rolls by the bytes alone); the ground grid needs nothing (the game's own 0x33-0x41
  packets, arbitrated by the host).
- What the host's bytes change is in the guest's block at once, but what the guest's game built from the block
  before (objects placed when an area loaded, a classmate shown) follows only when it reads the block again.
- A remote student's look (clothes) from its updated record shows when its model is made again.
- Classmates' decisions (who walks where) are not part of this: see "The town's people" below.

### The co-op side channel

`include/ntr/coop.hpp`: `ntr::coop_send(type, bytes, len, to = everyone)` and `ntr::coop_poll(handler, user)`,
messages of ours between the consoles of a session, reliable and in order from each sender, up to 64 KB each, beside
the game's own frames (which are not touched: the game's link carries one 16-byte packet per console per frame and
its movement packet fills it). Over UDP between PCs they are datagrams of a kind of their own on the LAN link's
socket, handed on by the host PC like the frames (`runtime/src/lan_wireless.cpp`); between copies on one PC a shared
ring of their own (`runtime/src/coop_local.cpp`, `/MagiciansQuest-Recomp-coop-1[-<NTR_LINK_NAME>]`). Reliability
(`runtime/src/coop_channel.cpp`): each sender numbers its datagrams per receiver, resends what is not acknowledged
after 150 ms (48 in flight), the receiver keeps what arrives past a lost one and acknowledges with a bitmap of it;
big messages go in fragments; a console that leaves the game's multiplayer ends its streams, and a stream the
receiver does not know (it started again) is started again with what was not acknowledged. Game code takes a message
type with `coop_sync::listen(type, handler)` (types 1-15 are the town sync's, others from 16; handlers run at the end
of a frame while the console is in the host's town). Type 40 is the players' pair messages (`game/src/coop_talk.cpp`:
talks, the pair's values, invitations, the meeting at Lovers Point, incantations; docs/COOP_RELATIONSHIPS.md). `tests/test_coop`: three PCs on loopback (to all, to one, guest
to guest through the host, 200 messages in order, a 64 KB one), a link losing a quarter of its datagrams and
repeating some, two copies on one PC (past a full ring), a copy leaving and one starting again in its slot.

### The town's people: classmates in a session (built, but little to keep in step yet)

`game/src/coop_npc.cpp`, on the side channel (message types 16-18). How the game runs its people, and what a session
changes (measured in a session from a day-one-done save, `--clock` on the next day 15:30 and the same night; RAM dumps
of the NPC entries, `*field+0x3E48`, 0x2B0 each, and gdb on the recompiled functions):

- **Brains**: placed in a session, every NPC gets the online brain (ov001 0x020F2F00, vtable 0x020FE0DC, its think a
  bare `bx lr`) on every console; outside a session the full one (ov002 0x02105D04, vtable 0x02106AFC). Seen in the
  dumps on both consoles.
- **Who is in town is the big difference, not the brain.** Single player at 15:31 on day two: 8 of the 11 town
  entries present (state +0xA2 = 1) at their own places, mostly at the student houses' doors. A session at the same
  time: 4 present (ids 52, 38, 16, 51), never near a student, and all four moving the same way at once (the same
  offset every dump, the same facing) at fixed places 16-48 units apart; the next day's night: none. The same 4 and the
  same motion on both consoles (a fraction of a unit apart). Giving every NPC the full brain in a session (research,
  done with gdb at the town's load) did not change it, nor did running the whole NPC director as in single player
  (`MQ_COOP_NPC_SOLO=1`: the field's session byte cleared for the director's update). What leaves classmates out of
  a session's town is in the placement of their start records (ov001 0x020F4AD0 and its predicates 0x020F47C0,
  0x020F42C8 (the *local* student's own variables), 0x020F4214, 0x020F488C, 0x020F4358, 0x020F44E0 (the hour),
  0x020F49D0, after 0x020F4900); not found yet. The data files are the same (`--trace-files`: NpcPlan, NpcRoom, NpcDir,
  NpcCmd and NpcCmdOM, NpcData, NpcTrg, relation and consider load in both).
- **Triggers do not fire in a session's town.** The trigger evaluator (0x020E3998) calls its player test (0x020E2C08)
  with the player index it keeps at director+0x2A4; in a session it would walk the players 0-3 (read in the code at
  0x020E3A2C, so a trigger can be about a guest too), but in the town the test was only ever called with -1 (gdb, a
  few minutes, both students walking near the school and the cafe), which in a session finds no student, and no
  trigger job (0x020E1950 from 0x020E3CE4) started on either console. With no classmate present there is nothing for
  a trigger to see.

What is built, for when the people are there (all of it only while the console stands in the host's town in a
session, `coop_sync::own_slot()`; hooks in `game/config/ynne/hooks.txt`):
- **The host decides the triggers**: `mq_npc_job` (ov001 0x020E1950 instead, the job start) on the host lets a
  trigger's job (return 0x020E3CE4) start, and if the game took it sends it (message 16: the NPC's id, the host's count
  of its jobs, the priority, the request's words, the actors it names (+4, +8: pointers) as a session slot or an NPC id,
  and the 16 trigger parameters at director+0x68C + i*16), noting whether the evaluator first cleared the NPC's job
  (`mq_npc_cancel`, ov001 0x020E6040 from 0x020E3C34). A guest drops its own trigger jobs (and that clearing), puts
  the host's parameters back, and starts the host's jobs through the same call before its director's next update
  (`mq_npc_director`, ov001 0x020E2978 before), with the actors found again on its side (`MQ_COOP_NPC_LOG=1` logs
  each). Jobs from players talking to NPCs, events and the plan are not sent: both consoles make those.
- **The rolls inside commands** (main 0x02005CF8 returning to 0x020C2F0E / 0x020C2F56, a random classmate;
  0x020CA282, a one-liner's variant; 0x020CFAD0, an item) are made from the NPC, the job's number and a count while
  that NPC's commands run (`mq_npc_cmd` / `mq_npc_cmd_end`, ov001 0x020E1E48 / 0x020E18F0), the same on every console.
- **A conversation starts on the console of the student it is about** (`mq_npc_op2`, command op 2, ov001 0x020E7918):
  the job's target is its request +4 when that is a student. That console runs op 2 (the game's own talk) and says
  when it is over (message 17); the others keep op 2 waiting (its status 1, as while their own student talks), at most
  3600 director frames, then go on (op 2 always ends with status 2, talk or not, so the scripts stay in step).
- **About once a second the host sends where its people stand** (message 18) when they have no job; a guest moves
  one that has none either, is in the same state and stands more than 2 units from the host's place
  (`MQ_COOP_NPC_NO_SNAP=1` turns it off). `MQ_COOP_NPC_OFF=1`: none of this (the game's own people).

**Verified**: the hooks are in the build and run in a session ("the town's people follow the host / this console"),
nothing else changes outside one (the quick regression). Not seen yet, for want of a classmate present near a
student in a session: a host's job started on a guest, the rolls from the job, op 2 on the target's console, a
classmate walking up to a guest. Next: find the session's rule in the start-record placement (above) so that the
classmates come out as in single player; then the jobs above have something to carry, and the wander target of the
full brain (§4.6 of the NPC research: ov002 0x020F321C, main 0x020214C0's two rolls) would be the host's too.

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
- **A side channel of ours** between the consoles of a session (above, "The co-op side channel"), and on it **the
  host's town kept the same on every console** ("One town").



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
   events. Between two players it is new content: new lines, new events, and their state beside each save. **Built
   during visits** (docs/COOP_RELATIONSHIPS.md): talking, the two-player incantations, the invitation to Lovers
   Point and the confession there, best friends or a couple, hanging out, back to friends.

Each milestone keeps the consoles together the game's way: what a student does is decided on their own console, and
anything another console must know or show travels in a packet (as the players' talk does, below).
