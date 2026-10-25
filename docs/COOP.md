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
11. **As long as you like**: a visit lasts until a player chooses Disconnect (Select). Only the new day (3:30 in
   autumn, 3:00 to 4:30 by season) interrupts it: the visit ends by itself, each of you saves, the day turns in each
   town, and you connect again by yourselves into the host's new day (see "How a visit ends").

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
without the room: query 0x2A in the dorm, then the same warp. While the room's loop runs, its
questions are answered as a player would, with the game's own pad: `mq_coop_pad` (main 0x02003628, right after
0x020036D8 has read the DS's buttons into the pad at `*0x020ACAA4`: +4 held, +8 pressed, +0xA repeated) adds Down/Up
until the menu's cursor is on the option above, then A, a press every 4 frames, and A for every other message. The
event marks where the loop starts and ends with two tests no variable passes (`if t0 == 0x4D510001/2`), seen by
the hook on command 0x38 (`mq_coop_gate`). It is queued (main 0x0201DAF0, as the town's triggers) once the student
is free to move: the field up with its controlled actor (`*0x020FE264`), no event running (script context +8) or
being started (+0x23BC, the start kind: the dorm's first-day intro starts from a start record some 30 frames after
the student appears) for 90 frames. Everything on the way (the save, the session, the warp, the arrival) is the
game's own; without a request nothing of it runs (the pad hook returns at once).

**Only in the dorm.** The room's loop calls into overlay 3 (ov001 0x020F1E28 -> ov003 0x021016C0 while it prepares
the connection, after "Saving."), the overlay the dorm and the room load into the slot where the town has overlay 2.
Run in town, that call landed in the middle of overlay 2's code (a return address there, 0x021016C1): `fatal:
interpreter left guest code at ffff0010, started at 021016c1` (the save itself was already written). So when the
student is free to move anywhere but the dorm (the field's area byte +0x3EB4 is not 14), or the game's overlay
table (main 0x02009DD8: slot of each overlay at 0x0209C988, overlay in each slot at 0x020A309C) does not show
overlay 3 loaded, Connect now first queues event 90011, the warp to the dorm by the Gateway (271's arrival), as the
DS has the player walk there; once free to move in the dorm it queues 90005 (3 tries, then it gives up). A guest
that finds no host is left in the dorm.

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
(No, then Exit) and stayed in the dorm; a host that asked from the school (after the principal's welcome)
connected and arrived in its dorm, and a guest then joined; both walked out of the dorm into the host's town together.
Since the dorm-first rule (from the day-one-finished save, `MQ_COOP_CONNECT_FILE`): a host from town at night (the
case that crashed), town by day, inside the café and in the school's hall each went to the dorm, ran the room and
connected; guests alone from town (night and day) and the school's hall went to the dorm and gave up cleanly; a
guest from town at night joined a host that had asked from the school's hall, both in the host's dorm; `fastgo.sh`
(from the dorm, no extra warp) still connects.

### How a visit ends, and the new day (built)

What ends a Multi-Card Play visit, found in the code (the session object is deleted only by ov001 0x020F15F0, from
the field's update when the session reports event 6 (ov001 0x020C9736: its message, then the field loaded again at
home, change 1) and from the session's end, ov001 0x020F16D8 (a field's end with change 6 or 8, ov001 0x020C9D7E,
and the communications room's own cancels); the field's update switches on the session's event byte, session+0x691C)
and seen live (`MQ_COOP_SESSION_LOG=1` logs the session's state, event and message bytes, flag byte 212, the field's
change and session bytes and the town's date on each change):

- **A player's Disconnect**: Select in a session asks "Disconnect?" (999:111, 112 Yes / No); Yes: "Disconnecting.",
  "Disconnected.", "Saving.", "Save complete." (115-118, the session's own loop, ov001 0x020EFC90) and the console
  goes home.
- **The host leaving**: every guest sees "The person you were playing with has disconnected. Returning to your own
  town." (110, ov001 0x020EF77C), then the same four messages: it saves and goes home. **A guest leaving**: the others
  see "<name> is going home." (202) and play on.
- **Errors**: a save that fails (session event 5: the field loaded again with change 6, which ends the session;
  ov001 0x020EFA86, 0x020EFCB8), a link lost.
- **Not the time.** No time limit (an idle visit stayed connected through 14 minutes and more), not the evening or the
  night, not midnight (connected at 23:55, still connected at 00:11 with the date turned), not the morning. There is no
  bed or sleeping in the game, and no saving in a visit: the Gateway offers only the communications room and Exit
  while connected (its Save, which returns to the title screen, is not offered); a visit saves when it starts
  ("Saving." in the room: the host with its town, mode 3; a guest its own data, mode 1; ov001 0x020EFA66) and when it
  ends.
- **But the day does not turn while connected.** The town's day is rolled (the day's map items, the classmates' day,
  overlay 27; the town's "last day" at town+0x9008) when the field's time (ov001 0x020C8CD4, once a frame) finds today
  after that day, today counting the night as the day before (until the season's first period, from the field's
  table at field+0x3E80, 8 bytes a season, row `((month - 1) - 2 mod 12) / 3`: 4:00 in spring, 3:00 in summer, 3:30 in
  autumn, 4:30 in winter): it loads the field again as a day change (change 4, ov001 0x020D17A4), and that load's start
  of day (ov001 0x020D5C14) stamps the date and rolls the day. In a session both are left out: the field's time tests
  the field's session byte (field+0x3EAF, 0x020C8E02) and the start of day the session pointer (0x020D5CFE); so the
  town stayed on the visit's first day until everyone went home (seen: connected at 3:29, the clock past 3:30 and
  6:00, the town's last day unchanged). It is not only a rule: the start of day loads overlays 25, 3 and 27, into the
  slots of the session's own code (overlay 23). Tried: with the session byte and pointer cleared on the host for those
  two calls, the day change started at 3:30 in the session and the game ended in the roll ("interpreter left guest
  code at 06000060, started at 0210f385").

**The new day for both together** (`game/src/coop_session.cpp`): once the host's town is due a new day (the rule
above, from the town's date and time and the table) and the host has been free to move for 2 seconds, the host
tells the guests (side channel message 41) and ends the visit with the game's own Disconnect, its buttons pressed as a
player would (`mq_coop_pad` calls `mq_coop_session_pad`: Select until "Disconnect?" shows, Up to Yes, A; the guests A
for "The person you were playing with has disconnected" and the messages after it). Each console saves and goes
home, where its own town turns the day as when alone; after 8 seconds free to move at home (the new day's own scenes
go first) each connects again with Connect now in its role (`coop_connect::request`), and the guests arrive in the
host's dorm, in the host's new day. `MQ_COOP_DAY_GAME=1` keeps the game's own frozen day (research).

Verified (two copies from day1done.sav, `--clock 2026-10-28T03:27:50`, Connect now): connected at 3:29; at 3:30 the
host logged the new day, "Disconnect?", "Disconnecting.", "Disconnected.", "Saving.", "Save complete."; the guest
"The person you were playing with has disconnected...", the same four messages; the host's town loaded again at home
as a day change (change 4) and its last day became the 28th (0x1B); both connected again by themselves and stood in
the host's dorm, the town's last day the 28th in both RAM dumps, the guest walking. The same flow by hand (the host's
Disconnect, then Connect now on both) works too. QUICK PASS (nothing runs outside a session).

Verified again with the wait at home: the guest's second arrival showed the dorm as a first one does. (Without the
wait, connecting again at once, the guest's touch screen kept a light blue band of the session's 2D pattern over
its top; the same Disconnect and Connect now by hand, with time between, showed none.)

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
talks, the pair's values, invitations, the meeting at Lovers Point, incantations, gifts (a recorded transfer:
room?, the item, in the bag or refused; ids still pending are settled when the two meet again); docs/COOP_RELATIONSHIPS.md). `tests/test_coop`: three PCs on loopback (to all, to one, guest
to guest through the host, 200 messages in order, a 64 KB one), a link losing a quarter of its datagrams and
repeating some, two copies on one PC (past a full ring), a copy leaving and one starting again in its slot.

### The town's people: classmates in a session (life as in single player, town and buildings; built and verified)

`game/src/coop_npc.cpp`, on the side channel (message type 18). What the game itself does in a session (found
statically, confirmed with RAM dumps of the NPC entries, `*field+0x3E48`, 0x2B0 each, and gdb backtraces on the
recompiled functions, from a day-one-done save with `--clock` on day two 15:30):

- **The game turns its people into a show for the visit.** At the session's start (ov001 0x020EF110 → ov003
  0x021016C0) it picks at most four classmates whose record says they are out (the location query ov001 0x020E76CC
  = 15) and gives each a fixed spot in a table at 0x020FE2A0 (16 bytes each: x, y, z, used). The town's placement
  (ov002 0x020FF134) shows those four there and hides everyone else at (115, 1); a per-frame tether (main 0x02030424)
  keeps them circling their spot (one 8-step circle for all, from the hour: "4 classmates moving together, never near
  a student", ids 52, 38, 16, 51 in the measurement). Single player's placement hides everyone too, and the plan then
  brings them out.
- **Everything that brings people out tests the field's session byte (+0x3EAF):** the director's init (ov001
  0x020E14C4) leaves out the day plan `/npc/NpcPlan.bin` (copied to director+0x882, 0x140 per NPC) and NpcParam; the
  town's construction (ov002 0x020FF828) leaves out the people's walkways (ov002 0x02104C18 on field+0x31BD0,
  registered by main 0x02048818, pointer at 0x020B8AC8; without them main 0x0201F81C answers "not here" to every plan
  move); a field's end (ov001 0x020C9AD4) does not write down where its people were (the list at *0x020FE29C, 12
  entries of 0x3C from +0x18, read by the next placement and freed by ov001 0x020C6F1C); each NPC gets the online
  brain (ov001 0x020F2F00, no think) instead of the full one (ov002 0x020FFDB4); the full brain's think (ov002
  0x02105F9C), the plan's kicks (ov001 0x020E6040 = start NPC i's job for the hour, 0x020E61F0, 0x020E6220) and parts
  of the director's update (ov001 0x020E2978) do nothing. In single player, NPCs come out because the director's
  round robin (0x020E5774) kicks each NPC's plan (0x020E6040 → 0x020E5EDC, the hour's plan slot → 0x0201F81C → the full
  brain's 0x02106558, a path on the walkways → 0x020E62B4); an NPC whose walk target (entry +0x38) differs from its
  place (+0x2C) walks (main 0x0201E5DC, state 0 → 1).
- The start-record checks (ov001 0x020F4AD0 and its predicates, 0x020F4900) are not about this: they choose a map's
  start script (`/script/start/map_ss.bin`, records of 0x50).

**Built** (all only in a session's field, `field+0x3EAF`; `MQ_COOP_NPC_GAME=1` keeps the game's own show):
- **The same as single player on every console, in the town and in buildings:** the plan and NpcParam load
  (`mq_coop_triggers` in game/src/cfie_hooks.cpp also clears its caller's flag in r5); the walkways are made before a
  placement (town: ov002 0x02104C18 on field+0x31BD0, *0x020B8AC8; building: ov003 0x021071F8 on field+0x451F4 with
  the area, *0x020B8AC4; both registered by main 0x02048818); the placements (town ov002 0x020FF134, building ov003
  0x020FEB54) run with the session byte cleared (`mq_npc_place`, `mq_npc_place_indoors`) except for the field's player
  slots they set up (ov001 0x020C5368, 0x020C5AF8: run with the byte as it is, `mq_npc_slots_a/b`; cleared, the town
  hung at its load) and the relations' drift they roll (ov001 0x020F5F08, 0x020F6B0C: left out as in a session,
  `mq_npc_drift`); a field's end writes down where its people were (`mq_npc_keep`); full brains (`mq_npc_brain`,
  `mq_npc_brain_room`), their think (`mq_npc_think`, `mq_npc_think_room`), the plan's kicks (`mq_npc_cancel`,
  `mq_npc_plan_one`, `mq_npc_plan_all`) and the director's update (`mq_npc_director`) run with the session byte
  cleared; the tether is skipped (`mq_npc_tether`).
- **One console decides each NPC (its owner):** the host owns every NPC except one nearest to a guest's student
  (nearer by a unit than the current owner's), which that guest owns; an NPC that is busy (a job, or taken by a
  conversation) keeps its owner. Only the owner kicks its plan, lets its triggers start jobs (the trigger evaluator
  tests the owner's own student, so a classmate noticing a guest, walking up and talking with op 2 happens on the
  guest's console the game's own way) and rolls where its brain wanders; on the others its think does not run.
  Twice a second (message 18) each console sends the NPCs it owns (place, walk target, state, busy), the host's
  message also carries the owners; the others' copies take the target and one more than 3 units away is put at the
  owner's place (`MQ_COOP_NPC_NO_SNAP=1` turns that off). In a field the host does not stand in, each guest owns its
  people. This replaces the earlier job replication (messages 16, 17), the host's turn-taking student test and the
  job-seeded rolls: a trigger's job, its rolls and its conversation all run on one console.
- Research switches: `MQ_COOP_NPC_LOG=1` (owners, triggers, placements, puts), `MQ_COOP_NPC_SKIP=place,walkways,keep,
  plan,brain,think,director,tether,owner`.

**Verified** (pairs from day1done.sav, `--clock` day two 15:30, Connect now):
- Classmates come out and go about the town by the plan; with the dorm reload at the session's start now writing
  down where its people were (the building runs as in single player), 9 classmates were out about 40 seconds after
  connecting (ids 72, 6, 65, 52, 68, 79, 76, 80 and one more; single player: 8 at 15:31), the session before this
  only 4. RAM dumps of host and guest taken together: the same NPCs out, in the same state, at places within about
  half a unit, with the same walk targets.
- Owners change as the students move ("NPC 51 now decided by slot 1" when the guest walked up to it, back to slot 0
  when the host's student was nearer), and a trigger's job started on its owner only ("NPC 52's trigger (command
  212) starts here" on the host).
- A building (the dorm at the session's start, a shop the guest entered) placed its people as in single player with
  its walkways made, full brains (ov003 vtable 0x0210A2BC) on both consoles, no crash; the shopkeeper walked up and
  greeted the guest on entering ("Hi there! I'm Vivian.", the shop's own event).
- Nothing changes outside a session (QUICK PASS).

**A classmate noticing a student** is still not seen, in single player either: the triggers' records
(/npc/NpcTrg.bin, 0x20 bytes each, grouped by kind = `[+0xC] >> 28`, conditions tested against a context
director+0x2A8 that the student test ov001 0x020E2C08 fills) start a command (/npc/NpcCmd.bin: 398 commands, op byte
and length per op); only commands 14, 18 and 321 have op 2 (talk to the student), and only records of kinds 2 and
3 start them (389: any NPC, command 18; 266: command 14; 424-438: NPC 6 and 15, commands 260/264 then 14/18). The
kind is the town's situation from ov001 0x020E5AC0: 1 normally, 2 while effect 0x1D is in the list main 0x0204433C
reads (20 slots), 3 while global store index 0x102 is set and index 0 is 2 (each also needs the record's other
conditions). Standing a student near classmates for several minutes (single player and session, at the café, the
bookstore, the dorm, 7:50 and 15:30) gave no op 2. A test that started command 18 on the guest's console as a
trigger would (the NPC taken, the job queued) did not walk the classmate to the student (its walk, op 6, takes its
target from the job's actor and timed out); that driver was removed. Next: find what puts effect 0x1D in that list
(a weather or an incantation), or which event sets index 0x102.

Open:
- Single player had 8 classmates out at 15:31 right after loading; a session now about as many, but the first
  field after connecting is the guest's own copy of the dorm reload: what a guest's people did before connecting is
  replaced by the host's places within half a second (puts of up to tens of units, once).
- Buildings were seen with no classmate inside at the time tested; classmates inside a building (school, café) at
  the same time on both consoles are not checked yet.
- Copies of a moving NPC lag the owner by up to a few units (twice a second); a put happens when they drift past 3
  units.

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
to register..." instead of "you can't use that while communicating"). The shops, on a guest (two day-one-done copies
connected, `--clock` a weekday 13:00; every text below on the guest's screen only, the host's money and bag
unchanged):
- **Daisy's** (234): *Buy berries* opens "So you want some berry seedlings?", the five berries, "They cost 100
  Ritch", *Buy*, "Thank you!": the guest's own money 3000 -> 2900 and the seedling (item 385) in the guest's bag.
- **The Emporium** (232): *Work* opens "<name>, do you really want to help me out around the shop?" and Malila's job
  (the materials she needs), not "No new jobs allowed during communication".
- **Harrows** (233, the CD floor upstairs: the elevator, then the right-hand door, area 46): *Buy a CD* ("CD orders
  are 1000 Ritch", *Place the order*, the song list, "So you'd like to order...", the guest's money 2900 -> 1900, the
  order in the guest's own record, s153 = 1); *Pick up the CD* (shown once the order has arrived; the arrival was set
  by a test poke, s153 = 2) gives "Here is The Rainbow March, the CD you ordered."

**Shopkeepers across the counter** (`game/src/coop_talk.cpp`, `talk_to_shopkeeper`, in the talk hook): in a session
nobody could talk to a shopkeeper standing behind their counter. Out of one, the shopkeeper walks about (the full
brain) and comes within the talk finder's reach (ov001 0x020C875C: 2.25 units for 126/127/128/129) of a student at the
counter; in a session every NPC gets the online brain, whose think does nothing, so the shopkeeper stays where the
shop placed them, 2.5 units from the counter's front (the finder's own numbers, Harrows: Piquard at z 24, the student
stopped at z 26.5), and pressing A did nothing. Now, in a session only, when the finder finds no one and the other
student is not in front, a shopkeeper (NPC 126-133) that passes the finder's own tests (a trigger, shown, not left
out, level, within 60 degrees, its own "can talk") within its reach plus 0.75 units is talked to with the talk call's
own steps (main 0x02028132-0x0202819A). Seen at Harrows (both floors), Daisy's and The Emporium. Still refused: the bell, the storage chest, the
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
