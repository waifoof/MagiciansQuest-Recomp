# Co-op

Where co-op stands, what the game itself offers, and the road from there to playing the whole game together.

## Playing together (from a new game)

1. **Connect the PCs** (each player: Settings > Network > *Co-op between PCs*). One picks **Host**, the others
   **Join**. On the same network, Join finds the host by itself; over the internet, type the host's public address
   in *Host's address* and have the host forward UDP port 49251 to their PC. Far apart (a round trip over ~20 ms, any
   internet link), both also set **Over the internet: On**: each PC then runs a hidden copy of the game standing in
   for the other player, so the game's own exchange (which needs a round trip well under ~20 ms) stays on each PC
   (two players; docs/COOP_INTERNET.md). Restart the game after changing it. (Two copies of the game on one PC need
   none of this.)
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
8. **Class together**: upstairs, Principal Sol waits by the classroom door (during a visit the game leaves the
   school's staff out; while connected they are there, `mq_coop_npc`). Each of you talks to him and picks a lesson
   (each your own: you may pick different ones). Whoever walks into the classroom first waits for the other ("<name>
   is on the way. Class starts when you're both here."); when the other arrives, class starts for both at once, and
   you see each other: the other student sits at the desk beside yours. Each answers its own questions and is graded
   on its own record. If the other student has no lesson to take, or isn't there within 90 seconds, the lesson
   starts alone. See "Lessons together" below.
9. **Mysteries together**: when one of you walks into a step of the week's mystery (a strange creature, a
   happening in town), the other is brought along: right away if standing nearby, otherwise asked "<name> has found
   something mysterious! Go and help <name>?" (Yes takes you there, into the same scene). Whoever is busy (talking, in
   a menu, in another scene) isn't interrupted, and the scene never waits. Each of you gets the scene's rewards; the
   mystery moves on in the host's town for both, whichever of you found it. See "Mysteries together" below.
10. **Talk to each other**: walk up to the other student, face them and press A (with no one else in front of you).
   They greet you and you choose: *Chat* (four topics in turn; the first chat of each day brings you closer), *How
   are we getting along?* (acquaintances, friends, best friends or a couple) or *Never mind*. On the other screen
   their student turns to you ("<name> came over to talk with you"), and is told about the chat. Each PC keeps the
   pair's record beside its save (`<save>.coop`, e.g. `cfie.coop`); see docs/COOP_RELATIONSHIPS.md.
11. **Closer**: cast Friendship or Declare love together (both of you at once, as the game asks in a session): it
   raises your friendship or affection by the game's own amount. Once you're good friends (and for a couple, with
   some affection too), the next talk offers an invitation to **Lovers Point** at a time you choose (10 a.m. to
   8 p.m.); the other player answers on their screen. Be there together within that hour and your scene plays on both
   screens; the one who was asked chooses a couple, best friends or "wait a little". Best friends and couples get
   *Hang out* in the talk menu, and *Go back to being friends* (with a confirm).
12. **As long as you like**: a visit lasts until a player chooses Disconnect (Select). Only the new day (3:30 in
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

### Turning off without saving (a visit never counts)

The game punishes a student whose game was turned off without saving: the warden's toilet cleaning ("School
Regulation, Item 99... Turning off the power without saving punishable by cleaning.", events 801-804 on maps 68-69,
while the student's s500 is 1; 802 block 2 in the dorm when it was left half done). How it knows (main and ov000):
loading a game at the title (ov000 0x020C7ACC) sets s500 from the slot's bit in the save's header byte 0x020AD821 (main
0x020106A8), then sets that bit (main 0x02010670) and saves, so the bit stays set in the saved game while it is played;
the Gateway's Save, which ends at the title, clears it (ov000 0x020CF72C, 0x020CFB64, ov001 0x020D61E0), and so does
the visit's start. The visit's end sets it again and saves (ov001 0x020F2874; the connect code ov023 0x02110CC8 and
ov024 0x021104BC also set it). Measured (two day-one-done copies connected, then the guest's copy stopped): the
host's RAM header byte was 0 during the visit, and the guest's save loaded with s500 = 0, no cleaning, so a game
closed *during* a visit is not punished by the game; the host's save after "Unable to receive data" (and 25 seconds
at home) loaded straight into the cleaning ("School Regulation, Item 99..."). So a Disconnect, the new day's end
(`coop_session.cpp`) or "Unable to receive data" do not punish by themselves, but a game closed after the visit's
end-save and before the student is home and free (during the end's or the error's messages, in the new day's 8
seconds at home before connecting again) or one that crashed then is punished at its next load.

Now (`game/src/coop_sync.cpp`, "visit"): while a visit is under way a marker file beside the save (`<save>.visit`,
e.g. `cfie.visit`) says so; it goes once the student has stood free to move at home, out of the session, for 10
seconds (the new day's reconnection, after 8 seconds at home, keeps it). A load at the title that finds it writes s500
= 0 instead of 1 (the store write main 0x0201E070 returning to ov000 0x020C7B18, seen in `mq_var_store`) and removes
it: `coop visit: the game was turned off during a co-op visit: not punished`. A game turned off without saving outside
a visit is punished as the game has it (`... (not in a visit): punished`). Verified: the host's save from after
"Unable to receive data" with the marker beside it loaded into the dorm hall, free to move, the log saying "not
punished"; the same save without the marker loaded into the cleaning. (`day1done.sav`, the test saves' source, has
the bit set itself: a copy started from it loads with s500 = 1, logged "(not in a visit): punished".)

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
  30 or 32: Friendship or Declare love is cast with the other student here, docs/COOP_RELATIONSHIPS.md; any other
  incantation's result id casts it through the game's own cast, main 0x02047A10, "Incantations in a session"; e.g.
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

#### Variables each console keeps, and the host's alone (built)

Some global variables are not "the town's" in the sense above (`kOwnVars` and `kHostVars` in
`game/src/coop_sync.cpp`; the lists were made from every event script's `set_town`/`add_town` writes, docs/COOP_GAME.md):

- **Each console's own** (the student's own business kept in the town's store): the lesson picked from Principal
  Sol's menu (g1017, written by ov038 0x0211C2B2; the classroom's start records match it), "lesson done today" (g1015,
  written by the lesson's end; it starts the after-class scene) and the after-class scene's sequence (event 010
  block 10: g1255 the step, g1253, g1254, g1257, g1259 what the following lessons branch on). A guest's writes are not
  sent to the host; the host's values of them are not written into the guest's copy (its bytes arrive, those bits keep
  the guest's); the host ignores them in a guest's request too. When a guest arrives in the host's town it gets back
  its own values from its own town (noted every frame while it is alone; the game's transfer had given it the
  host's). So each player picks and attends its own lesson, and neither one's lesson end clears the other's pick.
  House points (g1520-1523) still add up in the host's town.
- **The host's alone** (story and town progress a guest's scene could otherwise advance or finish without the host
  seeing any of it): the week's mystery (g1000 its number, g1001 its stage), the fields a new mystery clears (ov001
  0x020CE89C: g1002-1007, g1009-1012, g1070, g1242), the fields every mystery's scripts move (g1020, g1054, g1221,
  g1258, g1310-1321), those only mysteries' scripts write (events 071-122: g1021-1025, g1034, g1036, g1037, g1039,
  g1042, g1043, g1056, g1059, g1060, g1073, g1180-1191, g1195-1197, g1200, g1220, g1281, g1283, g1290, g1307, g1322,
  g1324, g1325, g1391, g1393, g1460, g1485-1499, g1575), the school festivals' done flags (g1387-1390; g1242 is also
  the week's event), and a classmate's transfer announced in homeroom (g1061: 1/2 the farewell, 11/12 the departure,
  21/22 the new student). A guest's writes are not sent; while an event runs on the guest it keeps what its script
  wrote (a script that reads back its own write sees it), and once no event runs the host's values are written back
  (`coop sync: the host's values back`); the host's own values of them that arrive while an event runs on the guest
  wait until it has ended (so a scene reads back what it wrote). A mystery's stage is moved on by the host's scenes,
  and a guest's own scene asks the host to take its stage writes ("Mysteries together" below).
- Variables 1256-1511 are single bits, eight to a byte (one byte holds g1256 the town's, g1257 and g1259 each
  console's own, g1258 the host's), so the lists are kept per bit (`var_bits`), not per byte.

Verified (two day1done copies connected by Connect now, `MQ_COOP_POKE_FILE`, RAM dumps of both): the guest's
g1017 = 7, poked at home before connecting, was given back on arrival (the host's was 0; the two town blocks then
differed only in the checksum, the time offset and g1017); the guest's pokes g1017 = 5 and g1015 = 1 stayed on the
guest ("each console's own, not sent"), g1001 = 3, g1061 = 12 and g1387 = 1 were not sent and went back to the
host's values (0, 11, 0) at once, g1520 = 9 reached the host as before; the host's g1017 = 2 was not taken by the
guest, its g1000 = 4 and g1001 = 1 were. The guest's own game wrote g1000/g1001 once by itself later (a screen
open): undone the same way.

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
of a frame while the console is in the host's town). Types in use: 1-15 the town sync, 16-18 the town's people (coop_npc.cpp), 40 the players' pair messages
(below), 41 the new day's end of a visit (coop_session.cpp), 42 the lessons (coop_class.cpp), 43 the mysteries
(coop_myst.cpp), 44 the incantations (coop_magic.cpp); a new sync takes the next free type and is listed here. Type 40 is the players' pair messages (`game/src/coop_talk.cpp`:
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

### Lessons together (built)

`game/src/coop_class.cpp` (docs/COOP_GAME.md, item 2). What the game does, found with RAM dumps of both consoles
taken together and gdb on the recompiled functions (pairs from day1done.sav, `--clock` 2000-01-18 9:30):

- **How a lesson starts.** Principal Sol's menu (ov038) writes the pick to g1017 and his event warps the student to
  the classroom, map 0 (event 010 block 99: `warp_map 0 (448, 0, 1152)`). Loading map 0 checks the start records of
  `start/mc_ss.bin` block 0 (ov001 0x020F4214, the record in r4): the first that matches starts its event; the
  lessons' records test g1017 (`event 041 block 1 g1017=1`, ... files 011, 021, 031, 041, 042), the homerooms' g1061
  (551, 555) and the festivals' announcements the date (500-503). The records are checked at the load only. There is
  no bell or time window for a lesson: the pick and the warp in start it. (Walking in through the classroom's door
  from the upstairs hall, the field showed area 1, not 0 [I: the classroom as a room].)
- **Seats.** Every classroom scene puts the local student at (448, 0, 1152) in 1/128 units = (3.5, 9.0) units, the
  teacher (actor 121 for the principal) on the platform at (5.8, 0.5, 1.8) and the three classmates (actors t131-t133,
  picked by query 0x0C from t128-t130) at the other desks; in the lessons t131 sits beside the student at (6.5, 9.0),
  t132 in front (3.5, 6.0), t133 diagonal (6.5, 6.0). During a lesson the student also goes up to the board at
  (2.7, 0.5, 2.0) (55 of the 65 lessons).
- **Why you didn't see each other.** Nothing hides the other student during an event: its console seats it at
  (3.5, 9.0) too, and its packets (types 0x0F and 0x11, the remote player's update main 0x02031404 case 0: +2 the
  area, +4 x, +8 z in 20.12 units, +14 the direction; sent every frame, during events too, with +3 the action, 7
  sitting) put it at exactly the same place as the local student, inside it.
- **The classmates were missing from classroom scenes in a session** (the empty desks of the first co-op lessons;
  the homeroom farewell, event 551 on days with g1061 = 11, waited forever for the leaving classmate on both
  consoles: the "Eric" box never closed). Three things, found with gdb (hardware watchpoints on an NPC's model link
  +0x20 in single player, breakpoints on the recompiled functions) and RAM dumps of both consoles:
  1. The event registers its classmates (actor_init, ov001 0x020D03C8: a list of four ids at field+0x3EA4), then the
     building's people are kept by ov003 0x020FF0D4: ov001 0x020C65FC ("visitors"), ov001 0x020C67F8 (field, n):
     keep the registered people first, at most n = field+0x3EAC, send the rest home; then a loop that makes the
     models of those kept (at most field+0x3EAC of them, ov001 0x020C5D20 -> main 0x020348DC). In a session n was 0
     (3 alone), so none were kept and none got a model.
  2. **Models in a session**: the field has four model slots for people (field+0xEEC + k * 0x83C, k = 1..4). In a
     session slots 1-3 belong to the other players (their objects' +0x20) and the loop and the visitors put every
     person in slot 4, the one the teacher uses: a classmate kept took the principal's model away (he vanished and
     the homeroom waited for him instead). With two players, slots 2 and 3 are free.
  3. The other console's NPC messages (coop_npc.cpp, the owner's places, "put at slot n's place") moved the scene's
     classmates away from their desks on the console that does not own them (the guest).
  **The same limit in town** (asked as a possible regression: "no classmates come out in town" against "9 out" in
  86d5d21's notes): clean builds of 86d5d21 and of the current branch behave the same, a pair at day two 15:30 warped
  out of the dorm, after 40 s: 3 classmates out in town on each console, only 1 with a model (slot 4), the same one
  on both (`town_old_*.bin` / `town_cur_*.bin` RAM dumps in the session's scratch, NPC 6 at (66.5, 28)). The earlier
  count was of NPC entries out, not of people drawn. Not a regression; town models are built now ("Classmates
  in town" below).

**Built** (only in a session; the same rules on every console, so no console has to be asked):
- **Starting together.** When a lesson's record matches on loading the classroom and the other student has a lesson
  picked (g1017 set, g1015 not) and is neither waiting in class nor in a class scene, the lesson is held: the record
  check reads g1017 as 0 for that record (`mq_class_hold`, from `mq_coop_var`) and our line says "<name> is on the
  way. Class starts when you're both here." (event 90014, line 420 of bank 60). Each console tells the other what
  it is doing twice a second and on each change (message 42: picked, waiting, in a class scene). The other student
  arriving sees this one waiting and starts at once (its record as the game does it); the waiting one then starts:
  our event 90015 warps it into the classroom again as Sol's event does, and its record, held no more, starts the
  lesson there (a lesson queued without the warp ran with the teacher hidden and the camera wrong: the placement that
  shows the scene's people runs on the load). It also starts when the other student has no lesson to take, is gone,
  or after 90 seconds; walking out of the classroom cancels the wait.
- **Each student its own lesson**: g1017 is each console's (above), so different picks give each its own teacher and
  questions in the same room at the same time, each graded on its own record (s229, s227, s228, house points).
- **Classmates in class** (`mq_class_summon`, `mq_class_model`, ov001 0x020C5D20 hooked): in the classroom in a
  session the placement keeps as many of the scene's classmates as there are free model slots (2 with two players):
  t133 first (a homeroom's story is about t133, the leaving or the new classmate), then t132, then t131, and never
  the one the lesson seats beside the local student while the other student takes a lesson too; the session byte is
  cleared while their models are made (put back with field+0x3EAC when that caller goes on to the plan's kicks,
  `mq_npc_plan_all`), and each gets a free slot (2, 3), not another player's nor the teacher's; a classmate the
  visitors bring in with slot 4 gets no model; a person whose place the scene already set is put back there after
  the model is made (the model's making puts them at the placement's spot, 128 units away); the owner's NPC
  messages are not applied while a classroom scene runs on this console; and a classmate a lesson seats beside the
  local student while the other student takes a lesson too leaves that desk ("leaves the desk beside to the other
  student").
- **Sitting together.** The other student's packets from the classroom's own seat (3.5, 9.0) in area 0 are shown at
  the desk beside (6.5, 9.0) (`mq_class_remote`, from `mq_coop_remote`), unless a classmate of this console's scene
  is shown there; packets from anywhere else (the board, the room before the lesson) are left alone, so the other
  student walks up to the board and back as on its own console. While the other student takes a lesson too, the
  classroom's placement leaves the classmate the scene put beside out of this console's room (`mq_class_summon`:
  "the desk beside is the other student's").
- Research switch: `MQ_CLASS_LOG=1` (every remote packet seen in the classroom).

**Verified** (two day1done copies, Connect now, `--clock 2000-01-18T09:30`, g1017 = 1 poked on both instead of
Sol's menu, the host warped into the classroom first, the guest about 20 seconds later): the host held its lesson
("lesson 4101 held until slot 1's starts too"), the guest started at once ("the other student: waiting in class"),
the host went into the classroom again and started; both read the same lines of the lesson a box apart, each saw
the other at the desk beside ("the other student sits at the desk beside" on both), and both lessons ran to "Well,
that should just about...". With different picks (host g1017 = 1, the principal's "Staffs & Wands"; guest 13,
Mr. Vlad's magic linguistics, event 011) both started together the same way, each with its own teacher and lines in
the same room, each saw the other at the desk beside; the guest was asked a question and answered it on its own
("That is correct..."), and both ran to their lesson's end. Screenshots: `shots/t4_*.png` and `shots/t5_*.png` of
the session's test harness (outside the repo).

**Verified** (classmates; same pairs): homeroom 551 with g1061 = 11, both walking in: on both consoles the principal,
the two classmates and Eric came in, Eric said goodbye and the homeroom ran to "That's all for homeroom!"
(`shots/t12_03.png`, `t28_*.png`; twice in a row after the last change; once before it the guest's leaving classmate
was another one, Tony, whom the placement had sent home: since then t133 is never left out); neither console played
it again (the host's g1061 = 12 reached the guest). A lesson together: the principal, a classmate in front of each
student, the other student at the desk beside, on both consoles (`shots/t26_3_lesson_12.png`). Homeroom 555 (the new student; host's
g1061 poked to 21): on both consoles the principal introduced the new student, who spoke ("My name's ... I'll be
studying with you from now on.") and the homeroom ran to its end once on each console (`shots/t29_03.png`). QUICK
PASS.

**Classmates in town (built).** The town's model manager, ov002 0x02100E30 (from the town's update, once a frame),
gives people the model slots base + i for i < field+0x3EAC, the base being 4 in a session (field+0x3EAF) and 1 alone,
and field+0x3EAC 1 in a session, 4 alone (slot k is the object at field+0xEEC + k * 0x83C; slot 0 the local student,
1-3 the other players, in the order of the player objects at field+0x3E4C, their +0x20). So a session drew one
classmate in town. What the manager does (decompiled; all of it is the game's, base and count aside):
- returns at once unless the field is the town's own (+0x20 == field+0x164) or 0x020AD7FF is set, and while
  field+0x3EAE is set with field+0x3E7C over 30; skips to the last step while the model tasks (+0x3E64, +0x3E74, main
  0x02004074) run;
- a box in front of the camera (ov001 0x020C6220: +4 x, +0xC z): x +-16 and 32 deep alone, +-14 and 28 in a session
  (ov002 0x02101350 tests a place in a box);
- (1) the slots in order: a slot without a person (its +0x370) is the free one (the last such); a person out of the
  box whose model may go (ov002 0x02101380: not busy, not the store's two companions 0xAD/0xAE) gives it back (main
  0x020348DC (slot, 0), field+0x31BC8 + slot = 0, ov001 0x020C87A0 (field, number, 0): the list at field+0x3EA4 of
  field+0x3EAC numbers) - one a frame;
- (2) with a free slot, the person at *0x020B8EF4 (when it has no model) is served first (a model asked for, ov001
  0x020C52D4 (field, slot, npc, 0): a task that makes it with ov001 0x020C5D20; its number at field+0x31BC4 + index);
- (3) else the NPCs in order: classmates (number 1-100) shown (vtable +0xA8 is 0) in the box and not marked (+0x2AA;
  marked when no slot is free, the resources are full, main 0x02037B34 = -1, or its +0x24's vtable +0x20 says no;
  unmarked when it leaves the box): one that had this index (field+0x31BC4) and no model gets it made again from the
  slot's resource (field+0xCAC + r * 0x24, r = index + 1 alone, 4 in a session; main 0x0200C3D4, 0x02034D2C,
  0x02034D94, 0x020348DC, then 0x020C52D4 kind 1), else a new one asks for the free slot - one a frame;
- (4) every slot with a person: near the camera or not (ov002 0x021013FC, +0x3A1 of the slot), shown when it comes
  near (main 0x02034F78, 0x020211C0), its size (vtable +0x28, +0xE3 << 7) and whether it is in a smaller box (x +-8,
  2 to 16 units ahead).

**Built** (`mq_town_models` in game/src/coop_class.cpp, ov002 0x02100E30 `instead`): outside a session (the field's
session byte 0) the hook returns 0 and the game's own function runs. In a session it is the same function rewritten
in C++, the same calls in the same order with the same tests and the session's box, except the slots it hands out:
those of the player objects whose session slot has no student (with two players 2 and 3) and 4, each slot k keeping
index k - 1 and resource k as alone (so the person-to-slot table and the resources stay consistent with the request
task's ov001 0x020C5D20, which loads into resource k). An absent player's object still holds its model (its +0x20,
the slot's +0x370 pointing back at it) until a person here takes the slot: for slots 1-3 such a holder counts as no
one (the first build gave it back as a person out of the box: its model went, and the next count thought the slot was
no longer free). A slot that stops being free (a student joins) gets its person's model back first. Present players'
slots are never touched, and the slot 4 rules of buildings (ov003 0x020FF0D4, ov001 0x020C65FC: one kept person or
visitor, slot 4) are not changed: buildings are their own fields with their own player bindings, and this manager is
the town's only. Research switches: `MQ_TOWN_MODELS_LOG=1` (each model given and given back, the slots every 600
calls), `MQ_TOWN_MODELS_OFF=1` (the game's own function in a session too).

**Verified** (pairs from day1done.sav, Connect now, RAM dumps of both consoles taken together, the slots' +0x370 and
the NPCs' +0x20): day two 15:30, both students south of the school: on both consoles slot 2 NPC 72, slot 3 NPC 80,
slot 4 NPC 76, the partner's slot 1 held by its player object (+0x20 = slot 1) on both; day two 7:40 by the school's
gate as the classmates arrive: slots 2-4 NPCs 79, 65, 80 on both, three classmates drawn on the host's screen and two
on the guest's (the third behind its camera), both students drawn (`shots/tm_d1_8.png` of the session's test harness, outside the repo; `tm_c4.png` the
15:30 one). The same
people get models on both consoles since the NPC sync puts the same people at the same places, but each console
decides with its own camera, so a person at the box's edge can have a model on one console only. Walking into the dorm
and out again (both), warps between arrival points, a lesson together afterwards (lesson 4101: held, the other at the
desk beside, the classroom's two classmates in models 2 and 3, both ran to "Well, that should just about...",
`shots/tmL1_*.png`), a host closed while the guest's town had a classmate in slot 3 (the guest got the game's
"disconnected" message and went home, no fault): all as before. QUICK PASS.
Not done: classmates in buildings in a session are still the game's (field+0x3EAC is 0 inside in a session: none kept,
the staff and a visitor in slot 4); the classroom's rule (mq_class_summon) could be widened to every building.

Open: a third or fourth student (fewer free models: one classmate with three players, none with four; the rules
follow one other student; in town the free models follow the session: one more classmate with three players);
the other student's answers are not shown on this console (each sees its own questions).

### Mysteries together (built)

`game/src/coop_myst.cpp` (docs/COOP_GAME.md, item 1). What the game does (event scripts 071-122, the start tables,
the matcher ov001 0x020F4A38; static research with the script decoder, then checked live):

- **The week's mystery** is g1000 (its number, 1-52 have scripts) and g1001 (its stage). Its scenes start from start
  records whose date bytes (+4, +5) are the mystery and the stage (ov001 0x020F42C8 compares them with store fields 0
  and 1, after student field 411 is 0): on loading a map (`mc_ss.bin`, block = the map, 100 the town: mostly nowait
  set-ups that place the creature, some real scenes), on walking into a trigger area (`map_u_ss.bin`, block =
  mystery * 10 + stage, the selector the area id of the town's event rects, `map_ss.bin` block 100), or at an actor
  (`act_u_ss.bin`, often an incantation cast at the creature: t160/t161, t163-t165, t200-t208 hold what was cast).
  Some blocks start from other scenes or code. The scenes move the stage themselves (set_town / add_town of g1001
  and the mystery's own fields) or the day roll does (ov027 0x02118CD4 copies g1025 into g1001); their rewards are
  the watching student's own fields and items. Which mystery runs is chosen at the day roll from
  `/script/start/obj.dat` (60 records of 12 bytes, one per mystery: a date range, or +10 a place in a rotation of 38
  kept in g1240); a field load clears a g1000 that the schedule does not hold (seen: a poked g1000 = 12 or 51 read 0
  after the next map load on the host).
- An event, started or queued, is noted at 0x020B8AAC (+2 its block, +6 its file: main 0x0201D0CC, 0x0201CEF4); its
  commands are at *0x020B8AB4; the script context's +8 is set while it runs.
- **In a session before**: each console ran a stage's scene only for its own student, when that student walked in;
  the other saw the student stand there. A guest's stage writes are the host's alone (above): not sent, undone once
  no event ran, so a guest's scene could not advance the host's mystery, and could start again for it.

**Built** (only in a session; message type 43):
- **The scene is told.** A mystery scene (an event of files 071-122 other than block 99, whose commands are not just
  a nowait set-up: `nowait_begin ... nowait_end, end`) starting on a console, however it started, is told to the
  other: the event, the mystery and stage it started at, the area, in a building or not, the student's place.
- **The other student joins** (each console decides from the message; no console waits for the other):
  - free (standing in the field, no event running or queued, not loading, for half a second) and in the same place
    within 12 units: the same scene is queued at once, there;
  - free elsewhere: asked "<name> has found something mysterious!" / "Go and help <name>?" (event 90016, Yes / No in
    t159). Yes: the student is taken there by events of ours whose command coop_myst fills in as it runs (the mark
    `if t0 == 0x4D510003`, seen by `mq_coop_gate`): into the town at the arrival point nearest the other student
    (90019, warp_local, as doors do), into another numbered map at the other's place (90017, warp_map), or within the
    same map a step beside the other (90018, actor_set_pos); then the scene is queued. No: the other console is told
    (logged). If the scene is over or the stage has moved by the time Yes is chosen: "Looks like <name> has already
    worked it out." (90020);
  - busy (an event, a menu, a talk, loading): never interrupted; the offer waits until the student is free and goes
    when the first student's scene ends (or after a minute), or when the town has moved past that stage;
  - a scene in a student's own room (dorm rooms 22-25, apartment rooms 26-29) is not offered; nor one in a building
    whose map is not known (some buildings show area 255 like the town; the town and the numbered maps are reached).
  - Walking into the same scene while the other's runs counts as joining it.
- **Rewards per student**: each console runs the scene for its own student, so each student's fields and items come
  from its own run.
- **The host decides the stage.** The host's own scenes write the town as usual (the town sync brings it to the
  guest). A guest's writes from a scene it joined are dropped (the host's run counts). A guest's scene that it started
  itself notes the writes of the host's variables it makes (the store write, main 0x0201E070, `mq_myst_store`) and
  sends them to the host when it ends ("result"); the host writes them into its town with the same store write if
  the town is still at the mystery and stage the scene started at and the host did not play that stage's scene on
  its own; else they are dropped ("not taken: the town has moved on" / "the host played this stage's scene itself").
  If the host joined the guest's scene, its own copy of the stage writes is undone first (the student who started the
  scene decides: a scene can branch on that student's own fields, an incantation they cast); a result arriving while
  the host's copy still runs waits for its end. The host answers (logged on the guest: "the host took our scene's
  stage").
- **Not again at the same stage.** After a scene, its start record does not start it again on that console while the
  town is still at the stage it started at (until the host's stage arrives, at most 20 seconds, longer while the
  other's copy still runs): the start records' date check (ov001 0x020F42C8, `mq_myst_date`, instead) answers "no"
  for it. While an event runs on a guest, the host's story variables keep the values its script wrote (coop_sync.cpp),
  and once it has ended the host's are back.
- **Side by side**: while both play the same scene, the other student's packets are shown a step (1 unit) to the side
  (`mq_myst_remote`, from `mq_coop_remote`; not in the classroom), as both scripts walk their students to the same
  spots.
- Research switches: `MQ_MYST_LOG=1` (every event start with its first command, set-ups, offers, the date check
  count), `MQ_MYST_GOTO=<path>` (write `<map> <x> <z> <y>` or `<arrival point> 0 0` there: the student is sent there
  by event 90017 / 90019, the way a join does; map warps made between frames into the town end the game).

**Verified** (two day1done copies, Connect now, `--clock 2000-05-24T10:00`, both taken to the school's front by
`MQ_MYST_GOTO` (arrival point 240), the mystery and stage poked on the host while in town (a map load clears an
unscheduled mystery), the creature's set-up block queued on both, scenes started with `MQ_EVENT_FILE` as their
records would):
- Near: the host's 12102 (mystery 51 stage 1, the creature appears; g1001 = 2, g1012 = 1): the guest, beside it,
  "near the other student: joining scene 12102 at once", both ran it; the guest's writes "not sent", the host's
  stage 2 reached it; both (51, 2) in RAM dumps.
- Asked, same map, guest's scene: the guest started 8204 (mystery 12 stage 2, the creature runs off; g1001 = 3,
  g1054 = 2, g1070 = 1, g1181 = 1) at the school with the host at the dorm's square, 38 units away; the host was
  asked, Yes, "a step beside the other student", the scene queued and run on both; the guest sent its writes, the
  host's result waited for its own copy to end, then "the guest's scene 8204 ... moves the town: g1001=3 g1054=2
  g1070=1 g1181=1; the host's joined copy undone"; both (12, 3).
- Asked, from the dorm (a building, area 14): the guest was asked about the host's 8204 in town, Yes: event 90019
  into the town (warp_local, arrival point 240 with the other's place), the scene queued on arrival and joined; both
  in the scene's field side by side.
- Busy: the guest in our demo's text boxes when the host's scene started: not asked; the host's scene ended first and
  the offer went ("it ended"). A Yes to a question still up after the scene ended: "Looks like AAAAAA has already
  worked it out." No: "no, not joining scene 8204" / on the host "the other student won't join this time".
- The date check hook is reached (tens of thousands of checks counted while walking in town).
- Screenshots (`/home/claude/run/live/shots/`, outside the repo): `myst_A1` (after the near join), `myst_B1` (the
  question), `myst_B3` (the host beside the guest during the guest's scene), `myst_C2`, `myst_D1` (both in the scene
  after joining from far / from the dorm), `myst_E1` (busy), `myst_F3` (already worked out), `myst_G1` (No).

Open: a stage scene started by its own start record (walking into the trigger area) was not reached in the test
(the trigger areas tried are on a lower level of the town than the arrival points, and map warps into the town end
the game); the scenes were queued directly, which is how the joining console starts them too. The re-start veto was
not seen acting live. A joining student whose own fields differ (an incantation not cast) may take another branch of
the scene than the starting one (its writes don't count then; the starter's do). Scenes in buildings that show area
255 are offered only when both stand there. Battles inside mysteries (Captain Dot, the Gremlin) are not covered.
More than two students: the rules follow one other student.

### Incantations in a session (built)

`game/src/coop_magic.cpp`, on the side channel (message type 44). Verified with a connected pair of day-one-done
copies (`--clock 2000-01-18T13:00`, outside the dorm), casting with `MQ_COOP_CAST_FILE` on either console.

**How a cast works** (static, then seen live): the touch screen's words end in main 0x02046FF8, which finds the
incantation's record (the table loaded from `/magic/magic_charm_comp.bin`, at *0x020B8E2C, 92 bytes each: +48 its
event file, +78 its index in the results table `/magic/magic_result_comp.bin`, 36 bytes each, whose +2 is the result
id) and writes the result id to flag byte 199; main 0x02047A10 then writes t202, applies the result (main 0x02045500:
the caster's own record, e.g. the incantation learned at +0x521.. and its count, and classmates' values toward the
caster's slot) and starts the event's block 1. The event shows the cast and does the rest. Result ids and events:
Rainbow 26 (310), Flower rain 29 (313), Rainmaking 50 (334), Friendship 30 (320), Declare love 32 (344), World Tree
85 (332), Phantasm 34 (350), Candyman 67 (352), Werewolf 64 (353), Homunculus 69 (355), Familiar 70 (356), Golem 71
(357), Charm 72 (359), Connect doors 31 (358), Sweet dreams 33 (345), Make peace 35 (321), Sit by me 36 (322), Honor
student 41 (346), Mushroom 66 (349). (Dryad, event 354, is in no record of the table.)

**What the refused ones do** (each cast alone, RAM compared before and after): none of the summons or transforms
leaves a creature or changes the student's model outside a scene: the cast's sparkle, a white flash, "You succeeded
in using the ... incantation!", and a few town variables, which are mystery fields and the host's (kHostVars):
Golem g1006 = 3, Familiar g1310 = 1, Candyman the mystery's stage (g1001 = 1, g1011, g1070; later g1001 = 2, g1054,
g1010) with its own scene with the candyman (actor 183). Werewolf, Homunculus, Charm and World Tree change nothing
else (World Tree: no town byte beyond the day's gossip list). Of query 0xB5's six: Make peace, Sit by me and Honor
student are the result applied to the caster (own record, classmates' values toward the caster's slot, which a
session keeps per console); Connect doors writes the caster's own s49/s50 (query 0x3F, with the game's own session
branch); Sweet dreams plays a dream scene for the caster (warp_back, a classmate in t202, back to where it was);
Mushroom writes the town's g1096 = 66 and g1121 = 1 (ordinary town variables).

**Built** (only in a session):
- **The refusals are lifted** for World Tree, Candyman, Werewolf, Homunculus, Familiar, Golem, Charm (their "if
  t212 != 0 go to <the refusal ending in 999:3120>" made a test that never holds) and Connect doors, Sweet dreams,
  Make peace, Sit by me, Honor student, Mushroom (the "if t0 == 0" after query 0xB5, when t199 is one of them):
  the caster's console runs the event as alone. Seen for each, cast by the guest (and Golem and Sit by me by the
  host): "You succeeded in using the ... incantation!" (Make peace and Sit by me: "You have properly prepared for
  ..."), both consoles still connected and free to move afterwards.
- **Phantasm stays refused**: its scene warps the caster away (query 0x14(1), warp_back) to meet the fortune-teller.
  The white screen seen when the test switch cast it was not a hang of the recomp but the game dying (the log ends
  in "fatal: no recompiled function at ..." inside the VBlank IRQ): the switch skips the magic menu, whose place test
  (main 0x02047650: charm record +70 must equal field+0x3EB5, the town 0 and the dorm 1; then +71 is the ground's zone
  under the student outdoors, ov001 0x020D5650, or a test of the area indoors) refuses Phantasm in the town and in
  the dorm. Cast in town anyway, warp_back reloads the town where the student stands, the scene then puts the
  student and the camera at the scene's own place (x 6, z 4: camera row 46 -> 7), and the town's cell-row cache
  (ov002 0x020FED08, 32 rows at town+0x29588) shifts by 39 rows: a copy with a negative length that overwrites RAM up
  through DTCM (seen with a hardware watchpoint: the VBlank queue pointer 0x020AD6D0 and the IRQ vector 0x027E3FFC).
  The game's own code on a state the menu never allows, so the same on a DS. Forced in the dorm it plays to its end
  (the fortune-teller, "You succeeded...", g1383 = 1) but leaves the camera at the scene's place. The switch now runs
  the place test and refuses Phantasm where it fails (`MQ_COOP_CAST_FORCE=1` casts anyway; other incantations log
  the test only). Where the test passes (records 8 and 11: a map with +0x3EB5 = 1, zones 5 / 4) was not found within
  the investigation, so the cast in a session is still unverified.
- **The end-of-spell cleanup** (query 0x16: ov001 0x020FAF14 on field+0x12930, the five words cleared) is skipped
  by the query in a session; for a lifted incantation it is called when its event ends ("the words cleared").
- **The other console sees it**: "<name> cast the <incantation> incantation!" (events 90021 / 90022, text lines
  440 + the incantation's index in `kMagicNames`, t155 = 20 + that index, t141 the caster's actor), shown once that
  student is free (no event, the field loaded; within a minute). When the caster stands in the same area, the
  student first turns to them and the cast's sparkle (effects 39 and 48, sound 539) plays at the caster's actor;
  elsewhere the line only. Seen on the host for each of the guest's casts and on the guest for the host's.
  Showing a summoned creature remotely was not needed: no summon leaves one outside its own scene.
- **The host decides the town**: a guest's lifted cast that changes the town (Golem, Familiar, Candyman, Mushroom)
  notes its writes of the host's variables (the store write, `mq_var_store`) and sends them to the host when the
  event ends; the host writes them if the week's mystery is the same (and the stage the same, or the one the cast
  itself moves to); the town sync brings them to everyone. Seen: the guest's Golem gave g1006 = 3, Familiar
  g1310 = 1, Candyman g1001 = 1, g1011 = 1, g1070 = 1 on both consoles; Mushroom's g1096 = 66, g1121 = 1 went as
  ordinary requests. Writes of the host's variables by the other incantations (Sweet dreams' scene writes g1242 = 0,
  a mystery field) are not sent (the town sync undoes them on the guest), so a cast cannot reset the host's mystery.
- **Rainbow, Flower rain, Rainmaking**: the game's own two-player path, unchanged. Verified in a session: the host's
  Rainmaking and the guest's Rainbow ended in "You succeeded in using ..." on both consoles, with the same town
  weather bytes, g1100 (50, 26), g1101 = 1 and the same weather byte shown (0x020A2EC0) on both. When the other
  student is not ready (just warped, walking, in an event) the game itself answers "The other person is not
  ready." (seen for Flower rain and Rainbow right after a warp; Rainbow again once both stood still: succeeded).
  After the other console's block 4 of Rainbow the script context's start kind (+0x23BC) can stay 2 with nothing
  pending (doors and triggers still work); our tests of "an event is about to start" use main 0x0201DF58 instead.
- **Weather**: the host sends its town's 20 weather bytes (+0x3F4, store indexes 540-559) as soon as they change,
  and a guest writes them into its town at once (seen with a poke of the host's bytes: "told the guests" / "the
  host's weather changed"). The guest's sky is drawn from them at its next area load; an immediate redraw was not
  built: ov001 0x020D8A0C (the town's call at load, with ov001 0x020C8B34's answer) turned out to take the day's
  sky phase from the date and time, not these bytes, and the function that draws the weather from the bytes at a
  load (ov001 0x020CFBC0, called from 0x020C8F5E) also rolls the guest's own weather, so it was not called.
- (research) `MQ_MAGIC_LOG=1` logs each if-variable test of the incantation events (310-361) with t199;
  `MQ_MAGIC_LIFT=<id>,...` lifts more ids' refusals.

### What a session refuses, and where (catalog)

Four mechanisms decide it:

- **A, menu text:** a tag `$016'A'B` in a menu option jumps to message B instead of A while in a session (main menu
  parser 0x02038938, the switch at 0x02038A9A). Used by the CD shop (buy/pick up, 5150), the general store's part-time
  work (5353), the flower shop's berry seedlings (5780) and the post office's Download (8300).
- **B, flag byte 212** (script variable 10212): `session[0x6912] + 1` while connected (set ov001 0x020F2300, cleared at
  the session's end, ov001 0x020EE0E6 / 0x020EE142 / 0x020EE1A6). Scripts test it with 0x38: the school bell (8614),
  registering in the insect and fish books (8634, 8649), the locker's explanation (8673), the storage chest (999:1021),
  the café (32:6230), one room door (999:5306), and the summon and transform incantations (3120: World Tree, Phantasm,
  Candyman, Werewolf, Homunculus, Familiar, Golem, Charm; all but Phantasm lifted, "Incantations in a session").
- **C, query 0x22:** in a session (variable 10020) and the console's slot (variable 10000), ov028 0x021152D0: the
  communications room (no second session), shopkeepers' walks.
- **D, query 0xB5** (ov028 0x0211A16C): the incantation's result id is flag byte 199. Connect doors, Sweet dreams, Make
  peace, Sit by me, Honor student and Mushroom are always refused (3114-3119; lifted in co-op); Rainbow, Flower rain and Rainmaking ask
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
own steps (main 0x02028132-0x0202819A). Seen at Harrows (both floors), Daisy's and The Emporium. Still refused: the bell,
the trash can (the "storage chest" below), Phantasm (the other summons and transforms are lifted: "Incantations in a session"), the bank's own session menu, Download, a second session.

**The tatami room** (`mq_coop_gate`, a second pattern): the room off the dorm hall (event 254 block 45: "if 10020 ==
1" (query 0x22, in a session), then "if 10212 != 0", both going to 999:5306 "The door is locked") now opens on its
own days (query 0x25: every third day of the month) and holds 3 students (query 0x26), as alone. Verified: on a guest
connected on the 18th, event 25445 walked it through the door into area 15 (the room with the fireplace, bed and
books), still connected.

**The "storage chest" stays refused: it is the dorm's trash can.** Event 225 block 1 ("if 10212 == 1, go to 999:1021";
flag byte 212 is 1 on a guest, 2 on the host, so only guests are refused) opens query 0x54, ov017's screen titled
"TRASH CAN" (the overlay's other screen, `/trashbox/chest_bg.bin`, is not opened by it). Lifted for a test, a guest
threw its Beige Wardrobe away: the item left the bag (the student's record +0x636, 15 items) and went into a list in
the *town* block (+0x644A, 15 items), which the guest kept as a change of its own ("kept here"), never sent. A guest
could take the host's thrown-away items back out of its copy (a duplicate), and its own would vanish when the host's
block is sent again. Lifting it needs the trash list made the host's (a guest's throw or take sent to the host as a
request, as the ground items are).

Script variables 10000+n are flag bytes n (`*(*0x020B8AAC + 0x14) + 0x2254`).

## What is built

- **Two copies on one PC** find each other (shared memory; Windows, and now Linux).
- **Two or more PCs** (`include/ntr/lan.hpp`, `runtime/src/lan_wireless.cpp`): the game's Multi-Card Play frames as UDP
  datagrams. One PC hosts (it hands each frame on to the others), the others join it, found by themselves on the same
  network or by address: Settings > Network >
  *Co-op between PCs* (Off, Host, Join), *Host's address*, *Co-op port* (49251 by default), or `--lan-host [port]` /
  `--lan-join <address[:port]|auto>`. Over the internet the host forwards that UDP port (only a short round trip
  works, see below). The window's title shows who is
  connected. Each PC's console gets a MAC address of its own.
  **Verified between "PCs" over UDP on loopback** (two copies, `--lan-host 49377` / `--lan-join 127.0.0.1:49377`
  and `--lan-join auto:49377`, Connect now; until then every co-op test ran over the same-PC link; recipe in
  docs/TESTING.md):
  - *Connect now*: from the dorm (day-one saves 130 s from start to both in the host's dorm on a loaded machine, a
    day-one-done pair 47-87 s); from town (both students outside, the café and the magic shop: each went to its dorm
    first, ran the room and connected, 39 s from the request). `auto` found the host by broadcast ("found the host
    at <the PC's own address>") and connected.
  - *The town sync and the side channel*: the whole block to the guest (39054 bytes), `var` pokes both ways (the
    guest's as requests, the host's in its next bytes); RAM dumps of the two town blocks taken together differed
    only in the checksum and the guest's time offset (and the minute byte when a dump straddled a minute).
  - *The players and the town's people*: a chat ("You chat with <name>..." / "<name> came over to talk with you",
    both `.coop` lines the same), a gift (the Beige Wardrobe moved from the host's bag to the guest's, "You gave the
    ..." / "You put it in your bag.", the transfer confirmed), and classmates in town matching (RAM dumps of the NPC
    entries: the same four out, places within 0.7 units, the same walk targets).
  - *A guest that stops and comes back*: the host said "Unable to receive data" ("lan: PC ... stopped answering"
    after 5 s) and went back to its dorm; the guest started again from its save and both connected again with
    Connect now (from town, and with `auto`), the town sync running with the new guest's node.
  - *A bad network* (`NTR_LAN_LOSS`, `NTR_LAN_DELAY`, testing switches in `lan_wireless.cpp`): with 5% of every
    datagram dropped (game frames and side channel, both PCs; ~5% counted dropped of 10762 sent) the pair connected,
    20 host pokes and 16 guest requests during walking arrived complete (the dumps' blocks again differing only in
    the checksum and time offset), and a chat worked; with 10% of the side channel's datagrams dropped (`NTR_LAN_LOSS=10:coop`, 203 of 2053 on the
    host, those also held 5 ms), the same 20 + 16 values arrived complete: the side channel's resends make up for the loss. **The game's
    own exchange is the limit, not ours**: with 7-10% loss on its frames it fails ("Unable to connect", "Unable to
    receive data"; a radio retries lost frames, UDP does not). Its host waits 25 ms (as melonDS) for the guests'
    replies to each command, about once a frame: a fixed 2 or 5 ms each way connected, 30 ms each way did not;
    15 ms each way with the wait raised (`NTR_LAN_WAIT_MS=80`, testing) connected but ran at 27 frames a second
    (123 without delay). **Frames that arrive out of order broke the exchange** (5 ms + 0-5 ms jitter: "Unable to
    connect"): a frame, command or ack older than one already received from the same console is now dropped as
    late (a radio never reorders), and that pair connected. So: a LAN, or an internet link with a round trip well
    under ~20 ms; farther, the Wi-Fi Connection way below.
  - Who is in the game's multiplayer was told only by one Begin or End datagram each; a lost one left the side
    channel without its peer (or with a stale one). Every keepalive (twice a second) now also says it, with a count
    of the sender's Begins and Ends so a late datagram never undoes a newer one (older builds' keepalives say
    nothing and are read as before).
  - Seen, not fixed (not about UDP): after a session ends with "Unable to receive data" the game gives the host an
    earlier copy of its student's things: a gift the host gave and that was confirmed came back into its bag (the
    guest, stopped before saving, had lost it, so nothing doubled here; a guest that had saved would keep it too).
    The gift records handle this only for gifts still pending.
- **Over the internet** (`docs/COOP_INTERNET.md`; Settings > Network > Over the internet, `--coop-internet`): each PC
  runs a hidden stand-in copy of the game for the other player, started from the other player's save, linked to the
  player's copy over the same-PC link; only game data crosses the network (the session's packets, movement
  newest-wins and the rest reliably in order; the side channel; the saves; each player's state, which its stand-in
  follows in and out of the session). Verified on one machine with four processes at 25 ms each way with 10 % loss
  and at 75 ms each way (with jitter and 5 % loss): Connect now, walking, a chat and a gift (each once), the town
  sync, a player leaving, the new day's reconnect, at the same frame rate as with no delay.
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
