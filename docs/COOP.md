# Co-op

Where co-op stands, what the game itself offers, and the road from there to playing the whole game together.

**Scope: two players.** Co-op is built and tested for two players (a host and one guest). Three or four (which the
game's own visits allow) are out of scope for now: no work goes into them, and gaps that only matter with more than
two players are not tracked.

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
  while connected (its Save, which returns to the title screen, is not offered; now also its destinations, see "The
  Gateway's destinations together", still without Save); a visit saves when it starts
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
- In a session the DS shows a 2D pattern (and a signal icon) on the top screen instead of the town (the 3D on one
  screen only, to keep the frame rate up while the radio runs). **Here the town is on the top screen in a session
  too, by default** (Settings > Network > *Top screen in co-op*: *The town* / *Pattern (DS)*, `online.coop_top_3d`;
  `MQ_COOP_DUAL_3D=0` / `1` overrides it): the field's screen setup (ov001 0x020C6F1C) is asked for both screens in a
  session too (`mq_dual_in_session`) and the session's camera swap is undone with it (see "Scenes' cameras in a
  session", "3D on both screens in a session").
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
  copy (the session's entry for that slot, +0x18) and fix its checksum. On each console the session's entry of its
  own slot points at 0x020AD824 itself (RAM: on the host slot 0 = 0x020AD824, slot 1 = 0x021984A0; on the guest
  slot 0 = 0x021984A0, the host's student, slot 1 = 0x020AD824), so a student's own writes and reads never go
  through a copy. Which record a student variable (< 1000, 20000-29999) is read from or written to is the getter's
  and setter's *player* argument (main 0x0201D960 r2, 0x0201D9F8 r3, resolved by main 0x0201DA9C): a session slot
  0-3 is that slot's record, anything else (the game's own checks pass -1, ov001 0x020F4224) the local student's.
  **Code of ours that reads a student's own variable passes -1**: a call with the argument left 0 reads slot 0's
  record, which on a guest is the host's student. (That was the "guest's s411 poked after connecting did not hold"
  of the Gateway tests: the record sync had carried the poke to the host's copy at once (`own record -> all: 1 bytes
  (+629:1)`), the guest's own record held 0, and coop_gate.cpp's s411 test read the host's 1. Fixed there, and in
  coop_class.cpp's announcement check, which read s508 the same way.)
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
(coop_myst.cpp; also the festivals' scenes, "Festivals and holidays together"), 44 the incantations (coop_magic.cpp), 46 the Gateway's destinations (coop_gate.cpp; 45 left free), 47 the insects (coop_insects.cpp), 48 the auditorium (coop_hall.cpp: the evaluation and the magic tournament), 49 the trash can (coop_trash.cpp); a new sync takes the next free type and is listed here. Type 40 is the players' pair messages (`game/src/coop_talk.cpp`:
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

### Insects together (built)

`game/src/coop_insects.cpp`, on the side channel (message type 47). Found statically (ov001, the disassembly) and
with RAM dumps of the insect manager on both consoles.

**How the game has them** [V]: each field has an insect manager at field+0xA74C (ctor ov001 0x020DBADC, update
0x020DC954 from the field update). When a town field is made (ov002 0x020FE990 → 0x020DC9CC) it notes the place
(+0x50E5) and the hour slot (+0x5110) and rolls `random(0,100) < 70` whether insects are out at all; if so it loads
`/map/insect/instbl.dat` (the four kinds for the place, month, hour; rain leaves out kinds 11-15), `insdata.dat`
(0x48 bytes per kind: +0 class 1-18, +1 count, +2 rate, +4 where it sits, +8/+0xA size, +0xC three parameter blocks)
and the kinds' models (state +0x50E4: 1 tables, 2 models, 0 running; the kinds at +0x50E0, four groups of five slots).
In its first 20 frames the update makes one spawn try per slot (0x020DBCC0, counter +0x5111): `random(0,100)`
against the kind's rate, a place from the town grid by the kind's habit (0x020DC140 / 0x020DC418, candidates then a
`random`), a size from `random(0,127)`; then the tables are freed. The 20 slots (manager+0x90) hold insect objects of
17 classes (0x18C-0x1B0 bytes; +0xFC place, +0xB4 offset, +0xC0 turn, +0x125 animation, +0x13C active, +0x140 kind,
+0x189 catchable, +0x120 the student it watches). The base update (0x020DD280) lets an insect think (vtable +0x2C
sensor, +0x30 think, with the RNG) only within 10 units of the student it watches, and shows it only then. Each
console rolled all of this alone: the partner saw other insects in other places, or none.

**The net** [V]: X uses the held tool (R and L switch tools; the net is item 662). The net's state function main
0x02029F2C (student +0xA5 step: 10-29 a swing at what is in front, 30-59 a swing through the air, 60 caught,
100 missed) calls the catch test main 0x0202B4D0 (student, reach, kinds): for each slot an active, catchable insect
hit by the net is the student's (+0x5DC = slot, the insect inactive); then main 0x0202B3D4 puts it in the bag (item
2110 + kind, the size in the high bits) and the hand, the scene "You caught a ...!" follows and the insect book is
written by the game's own flow. Only the local student (+0xAA = 1) tests; the partner's swing is only shown.

**In a session** the host's console decides:
- The host runs its insects as the game does, except that each insect watches the student nearest to it, its own
  or a guest's (the remote student objects, field+0x3E4C; another one takes over only when nearer by a unit: two
  students about as near trading places every frame looked to the sensor like a sudden approach, and it flew off).
  So insects near a guest move, and fly off when the guest comes close, on the host's console. Every 6 frames it
  sends the manager (place, hour, kinds) and the insects that changed (slot, a number for the insect, kind, size,
  place, offset, turn, animation), all of them every 60 frames; an insect caught or gone is left out.
- A guest standing in the same field mirrors them. Its own spawn tries are left out; when its manager's place, hour
  or kinds differ from the host's (or it rolled none, or its tables are gone) it is reset (0x020DBB48) and loaded
  again for the host's (0x020DC648; the host's kinds written as the models are loaded, 0x020DBBDC). Each of the host's
  insects is made in the same slot by the game's own spawn try (0x020DBCC0) with the slot, the rate passed, the
  host's place (the place pickers answered) and its size; one the host no longer has is deleted. Its copies do not
  think (+0x184 set while the base update runs): each frame they move a quarter of the way to the host's place
  (snapped beyond 4 units) and take its turn, offset and animation; the animation plays on the guest's console.
- **A catch is decided once, by the host.** The host's own catch stands at once; its insect is left out of the next
  state and the guests' copies go. A guest's catch test is run and undone, and the host is asked (the insect's slot
  and number); meanwhile the guest's swing holds at the miss it would have ended in (step 100, the net's state
  function skipped). The host says yes when the insect is still there, active and catchable and not its own
  student's, and deletes it; no otherwise. On yes the guest's console makes the catch the game makes (the catch
  test's writes, 0x0202B3D4, the catch animation, step 60): the bag, the book and the message are the guest's own.
  On no the swing ends as the miss and the copy is no longer catchable there. The host answers in the order asked,
  so two swings at one insect give it to the first that hit.
- In a field the host does not stand in (no state for 90 frames) a guest's insects are its own, as alone. The
  host's insects it made stay (its spawn tries are not run over them) and think on its console from then on. A state
  counts only for the same map *and* the same field class (the field's vtable) and a manager with a place: as the host
  went into the bookshop, one state from its building being made said "town, no kinds" and emptied the guest's town.
- **Mushrooms** [V, static]: the town's mushroom manager (a stack object of ov002 0x020FF828, ov001 0x020D455C) clears
  the old mushrooms (0x020D4490) and places the day's by trees (0x020D41FC) in the *town grid* (+0x438 of the town
  block, items 256-279), once per period (store variable 198 holds the period it last did). The grid is the host's
  (the town sync), so a guest's own clearing and placing are left out (`mq_mush_place`) and its write of variable 198
  is not sent to the host as an action of the guest (coop_sync.cpp, kMushroomDay): the host's placing, when it
  enters town, reaches the guest as changed grid bytes. Seen: the same 31 mushrooms in the same cells on both
  consoles (RAM); the placement itself did not run in the tests (July and October from day1done.sav).
- **Fish**: next section.

Verified (pair from day1done.sav, `--clock 2000-07-18T14:00:00`, Connect now, both warped to the bookshop, `e 243`;
RAM dumps of both managers; MQ_COOP_INS_LOG=1; the students put beside an insect by poking the local player object's
place, field+0x383C+0x2C/+0x34, and the net taken with R):
- Spawn: the host's 11 insects (kinds 0, 1, 11, 12; place 1, hour 1) on the guest in the same slots, kinds and sizes,
  the places identical to the 1/4096 unit; the guest's own manager had rolled none or other kinds before ("the manager
  made the host's").
- Behaviour: insects a guest walked past flew off on both consoles (slot 10: (27.32, 65.84, 68.50) on both, 1 frame
  apart; flying butterflies 0.07-0.5 units apart, the guest's copy gliding behind).
- A guest's catch: "our net caught insect 12: asked the host" → host "slot 1 asks for insect 12: yes" → guest "insect 12
  is ours: caught", "You caught a Large Brown Cicada!", item 18505 in the guest's bag; the insect gone from the host.
- A host's catch: "You caught a Mingming Cicada!" on the host; the guest's copy gone ("insect 16 gone at the host").
- Both swinging at one butterfly (the guest 3 frames first): both swings started (step 10 on both); the guest asked
  first and got it ("You caught a Small Cabbage White!", item 2110 in its bag), the host's swing missed and its bag
  had nothing new. With the host first, or on cicadas, the insect flew off before either net (both consoles agreed
  on where it went).
- The host going into the bookshop (`e 244`) for 400 frames: the guest's 11 insects stayed (the same objects, the
  same places), and the guest kept running.
- `quick.sh`: QUICK PASS.
- Seen, not ours (the same with `MQ_COOP_INS_OFF=1`): a host *warped* back into town by `MQ_WARP_FILE` while the guest
  stands in town hangs the guest (its game waits on the wireless link; frames count, the field does not run). Tests
  warp the guest out first, or leave the host indoors.
- Open: cicadas on trees often flew off as soon as the students were put beside them (3 of 3 with both students, 1 of
  3 with one; the class think's sensor, 0x020DCFEC, sets "near" from the watched student's distance and height).
  Whether a second student, or the poke (a jump, which the other console shows as a dash), scares them was not
  looked into; walking up was not tried.

### Fish together (built)

`game/src/coop_fish.cpp`, on the side channel (message type 50). Found statically (ov001 0x020D1BD0-0x020D3B60, the
rod in main 0x0204D890-0x0204F98C) and with RAM dumps of the fish manager on both consoles (`MQ_RAM_DUMP`).

**How the game has them** [V]: each field has a fish manager at field+0xF87C (ctor ov001 0x020D1BD0). When a field
is made, 0x020D2748 (manager, student, town time, place) loads the place's four kinds (+0x2C61, `fishtbl.dat`; the
hour slot +0x2C60), rolls 32 spawn points by the water (0x020D1D4C, +0x2C80, 16 bytes each: place, kind, state 1
free / 2 taken; eight per kind) and up to eight fish at once (0x020D1EAC, `random(0,99)` against the kind's rate;
+0x24, 0x1E4 bytes each, vtable 0x020FD4C8: 16 sizes +0xD0, size class +0x120, kind +0x122, the size picked +0x125,
point +0x127, place +0x1A0, home +0x1AC, think state +0x1C8 / count +0x1CA / length +0x1CC, the student it watches
+0x1D0, active +0x1DC, ripples +0x1E2). The manager update (0x020D24C4, from the field update after the insects')
runs each fish (vt+0x34 0x020D3098): within 18 units of the student, as |dx| + |dz| (main 0x02035938, the game's
distance everywhere here), it is shown (+0xCC) and thinks (0x020D2D5C): every 80-120 frames (40-60 in a session) it
jumps to a spot within a unit of its home (0x020D2E18) and, while a rod is out (+0x1E2, set by the rod through
0x020D2A10), leaves a ripple where it was (0x020D2EA0, effect 27). The fish themselves are not seen (they are drawn
1.125 units below the water); the ripples are.
- **The lure** [V]: X uses the held tool (R and L switch: hand, shovel, rod, net, can). The local student's rod
  (+0xA4 its student, +0x2E0 the manager) gives the manager the lure's place (+0x2C6C, 0x020D29FC) and the student's
  (+0x2C70). Each X while the lure is in the water reels it in a little and pulls a trigger (+0x1140, 0x020D29F0;
  it is also the hooked fish's +0x1FC). The update counts the triggers that found a shown fish within 4 units of the
  lure (+0x2C74, the reach at ov001 0x020FD424); at 2 the nearest fish becomes the **hooked fish** (+0xF44, vtable
  0x020FD488: its kind and sizes, put 0.75-1.25 units beside the lure, 0x020D3A94) and its slot is kept (+0x2C7E).
  The hooked fish's think (0x020D32C8, run each frame at the end of the update; its state +0x1C8 = manager +0x110C,
  active +0x1DC = manager +0x1120) waits (2), comes (3) after a trigger, touches the lure (4), bites (5: the lure
  sinks) after a trigger within a distance by its kind (ov001 0x020FD442 / 0x020FD44E), and leaves (1) when the lure
  is over 4 units away or it loses interest (+0x1F6). While it is active the other fish stand still. The rod plays
  the strike and the reeling; a catch puts the fish in the bag (main 0x0204F880: the item from the kind, 0x020D27E8,
  the size in the high bits) and 0x020D2958 moves the slot's fish to another free point of its kind; an escape
  (main 0x0204DCD0) leaves it where it was with new timers. Kind 66 is caught once: 0x020D27E8 retires every fish
  of that kind.
- Left alone in a session each console rolled its own fish (the RNG is the console's) and the partner's lure meant
  nothing to them.

**In a session** (both consoles in the same field; a guest's own spawns are replaced):
- **Owners.** Each of the eight slots has an owner whose console runs it as the game does: the host, for all of
  them, unless it gave one to the guest. The other console's copy does not think: its active flag is cleared while
  the manager update runs (so the game skips it and no lure finds it) and it takes the owner's record (kind, sizes,
  place, home, point, think state; 0x10E bytes from the fish), put in place each frame; shown by the game's own rule
  and animated (main 0x0200CD04). Where the copy had no fish the game's own 0x020D31AC / 0x020D314C make one first.
  The host's spawn points go to the guest too (when they change and every 60 frames), so the points a catch moves a
  fish to are the same. Records go every 6 frames for the slots that changed (the think count alone is not a
  change), all of them every 60 frames.
- **A guest's lure.** The host's fish are not in the guest's own search, so this file does it ahead of the update
  with the game's rule (a trigger pulled, a shown fish within the reach, the nearest), keeps the game's count for it,
  and at the second count holds the trigger back and asks the host for that slot. The host says yes unless its own
  hooked fish came from that slot (or it is still the host's lure's), and from then on the guest's console runs it
  (the copy takes the host's latest record first); no otherwise (the guest leaves that fish alone for two seconds).
  On yes the guest pulls its trigger again with only that fish in reach, and the game makes the hooked fish from it.
  So a fish is hooked by one lure only: the host's search never sees a slot it gave away, the guest's never makes a
  hooked fish from a slot it was not given.
- **The hooked fish on the other console.** While a console has a hooked fish, the record of the slot it came from
  carries the hooked fish's place: the other console's copy swims there (a quarter of the way each frame). The
  ripples and splashes of a console's fish and of its hooked fish (0x020D2EA0: 27, 23, 22, ...) are made at the
  copy on the other console too (message 5), and while anyone's lure is in the water every console's fish make
  their ripples (+0x1E2 set for the update).
- **Giving back.** Once the guest's hook is over (the hooked fish gone and the slot no longer the lure's; 30 frames,
  at least 60 after the grant) the guest sends the slot's record back: the host takes the place, the timers and the
  point (its spawn points: the old one free, the new one taken, when a catch moved it) and runs it again.
- The catch itself, the bag, the book and the message are the game's own on the console that hooked the fish.
- A new field on the host (a new number, the "epoch") makes every slot the host's again on the guest; a guest whose
  host is not in its field (no record for 90 frames) runs its own fish, as alone; the host takes back a slot when
  the guest sends nothing for 90 frames.
- `MQ_COOP_FISH_OFF=1`: none of this. `MQ_COOP_FISH_LOG=1`: what it does. `MQ_FISH_TRACE=1` (research, also alone):
  the lure, the trigger, the count, the hooked fish's state and place whenever they change.

Verified (pair from day1done.sav, `--clock 2000-07-18T14:00:00`, Connect now, both warped to the bookshop, `e 243`,
then walked to the river east of it, (40.4, 74.6); RAM dumps of both managers; logs):
- Spawns: the guest's six fish (kinds 2, 2, 3, 7, 34, 34; slots 6-7 empty), their points, sizes, places and homes
  identical to the host's to the 1/4096 unit; the 32 spawn points identical.
- Thinking: the host's fish 2 moved on from (42.90, 71.11) (x 44.40 a few hundred frames later); the guest's copy at
  the same places in each pair of dumps.
- A host's hook: "a fish from slot 2 goes for our lure", the host's hooked fish at (43.22, 72.39) and the guest's copy
  of fish 2 at (43.22, 72.38).
- A guest's hook: "our lure found fish 2 (2.85 units): asked the host" -> host "slot 1 asks for fish 2: yes" -> guest
  "fish 2 is ours", "the lure's trigger again", "a fish from slot 2 goes for our lure"; the guest's hooked fish at
  (42.83, 71.50), the host's copy of fish 2 at (42.84, 71.50). The fish swam off (the lure came in): "fish 2 given
  back (point 8, at 44.30, 70.85)" -> host "fish 2 is ours again"; both consoles then had fish 2 at (44.30, 70.85),
  point 8, its new timers.
- `quick.sh`: QUICK PASS.
- Not seen in a test: a catch in a session (the bite needs the lure still and one X at the right distance; a test
  driver reeled the short river casts (3.9 units) back in before it), so a guest's catch moving a fish to a new point
  on both consoles, and both students' lures at one fish together (the second lure's ask answered no), are open to
  verify. The catch and its writes are the game's own and unchanged.

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
- **Homeroom too** (551, 555: records testing g1061, before the day's lesson) is held the same way (its record reads
  g1061 as 0 while it waits, and the lessons' records after it wait with it); and a lesson or homeroom also waits for
  an other student who stands in the school (the hall, area 6, or the classroom as a room, 1) with no lesson done
  today (state bit 8 of message 42), not only for one who has picked already: in play the second player is usually
  still walking up to Principal Sol (docs/COOP_PLAYTEST.md).
- **The classroom's model loop** (ov003 0x020FF0D4 -> ov001 0x020C5D20, `mq_class_model`) takes the room's
  classmates in NPC-list order, not the people the placement kept (field+0x3EA4): a classmate the scene does not
  have, or one that already got its model, gives its turn to a kept one still without a model (seen: homeroom's
  leaving classmate without a model, the farewell waiting for ever; then one classmate made twice in 555); in a
  homeroom t131 and t133 stay (its query 0x0C brings them in before the scene swaps the two).
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
    (90019, warp_local, as doors do), into another numbered map at the other's place (90017, warp_map; the place
    in the scripts' frame, "Where beside is" below), or within the same map a step beside the other (90018,
    actor_set_pos); then the scene is queued. No: the other console is told
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
- **Where "beside" is in a numbered map.** A script's place (warp_map 0x2D, actor_set_pos 0x02) and the student's
  position (0x020B8B7C) are in different frames in many numbered maps: the position is the script's place plus an
  offset of the map (the beach, 64: (0, 11.85), warp_map (5, 14) lands at (5, 25.85), actor_set_pos (8, 30) at
  (7.3, 41.85); the dorm, 14: (0, 8.22), (15, 3) lands at (15, 11.22); the town agrees, and its joins use arrival
  points anyway; the game's own movement packets carry the scripts' frame: on the beach (9.31, 21.19) for a student
  standing at (9.31, 33.04)). The joins had warped to the other's *position*, so on the beach the partner landed
  11.85 units off (in the surf), within kNear, so no step followed. Now each console learns its maps' offsets from
  its own warps: the place a warp asks for is caught at 0x020AD7F0 each frame (as coop_gate.cpp does), and the place
  the load puts the student at (the position's jump in the new map, before anything pushes it: the dorm's warp to
  (15, 1) lands inside the Gateway at (15, 9.22) and is pushed on to (15, 10.74). The position lags: from the dorm
  into Lovers Point it still read the dorm's (14.95, 10.74) some 30 frames after the student was free in the new map,
  so only the jump counts; with no jump of half a unit, nothing is learned) gives the offset (`coop myst: map
  64: a script's (5.00, 14.00) is the position (5.00, 25.85): offset (0.00, 11.85)`). The scene's start message
  carries the starter's offset for its map (bytes +24..+36; a message without them still works), and a join warps or
  steps to the other's position less that offset, or less its own one of that map. Known to neither (a student that
  came in some way that left no warp place to catch): the join warps to the position, waits for the landing (nothing
  is judged from the old place the position still shows while the new map loads), learns the offset from it, and if
  it is more than 3 units from the place asked for, warps there again (a warp_map within the map, not a step:
  actor_set_pos walks the student there over a few frames and can cross a trigger area, as a step from the beach's
  surf crossed the exit to town). `MQ_MYST_FORGET=<file>` (research): while the file exists the offset is not sent.
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

**Verified, "beside" in a numbered map** (two day1done copies, Connect now, `--clock 2000-01-18T10:00`, the host
sent by `MQ_MYST_GOTO` (a warp_map event, as a join's), the host's scene started with `MQ_EVENT_FILE` = 50003 (Miss
Strawe's talk: a festival scene, so the join is "go along", and no stage moves to end the join early), the guest in
the dorm answering Yes; positions from `MQ_POS_FILE`):
- The offset sent (the host had warped into the beach, 64, at (5, 14) and stepped to (6.32, 27.85)): the guest
  "to the other student's map (map 64, 7.3, 16.0; the other's place less its map's frame offset)", one warp, landed
  at (7.31, 27.85), one unit beside the host. A warp to the host's position itself (as the joins did before) lands
  11.85 units off: (6.00, 37.70) for a host at (5.00, 25.85), in the surf.
- The offset withheld (`MQ_MYST_FORGET`), a map the guest had never been in: Lovers Point, 49 (offset (0, 12.23)):
  the guest warped to the host's position, landed at (7.99, 26.65), "landed 12.2 units from the other student's place
  (map 49's frames): warped there again", and stood at (8.00, 14.42) beside the host at (7.00, 14.42)
  (`shots/bf_myst_C2.png`: both students under the bell on both consoles). The same on the beach
  (`shots/bf_myst_A.png`, both on the sand; that build stepped instead of warping and the step crossed the beach's
  exit to town on the way, hence the warp).
- Offsets learned: the dorm (0, 8.22), the beach (0, 11.85), Lovers Point (0, 12.23).
- QUICK PASS.

Notes for the trigger areas (not finished): the town's event rects are `/map/event_rect/event_rect_comp.bin` (LZ10;
loaded by ov001 0x020F5654 from the field's set-up, ov001 0x020C7296): a header of 2 sections (0x20 and 0x23D0); the
first has 70 entries (offset, size) of 24-byte rects (x0, z0, x1, z1 in 20.12, then two words: a code and a
direction), the second 249 entries (cells?) of the same kind; the point test is ov001 0x020F5894 (x at +0, z at +8
against the rect), its caller ov002 0x02103468 first makes the student's position local with ov001 0x020D57EC.
Which entry is a map_u_ss selector (e.g. mystery 12 stage 2, 8204: selector 17) and where its cell lies in town was
not worked out (a walk from the school's arrival point toward the guess stopped at a fence).

Open: a stage scene started by its own start record (walking into the trigger area) was not reached in the test
(the trigger areas tried are on a lower level of the town than the arrival points, and map warps into the town end
the game); the scenes were queued directly, which is how the joining console starts them too. The re-start veto was
not seen acting live. A joining student whose own fields differ (an incantation not cast) may take another branch of
the scene than the starting one (its writes don't count then; the starter's do). Scenes in buildings that show area
255 are offered only when both stand there. Battles inside mysteries (Captain Dot, the Gremlin) are not covered.
More than two students: the rules follow one other student.

### Festivals and holidays together (built)

`game/src/coop_myst.cpp` (the same messages, type 43), `game/src/coop_class.cpp` (the announcements), docs/
COOP_GAME.md item 3. What the game does (static: event files 500-505, 101, 110 with the script decoder, the start
tables; checked live where noted):

- **The four school festivals** are each announced before class the month before and held the 2nd-8th: 500 Miss
  Strawe's spring tea party (Apr 9-30 / May), 501 Mr. Vlad's test of courage (Jul / Aug; its haunted house is
  mystery 37, file 107), 502 Mr. Oakley's mushroom hotpot cookoff (Sep / Oct, Fungi Forest, map 66), 503 Principal
  Sol's rainbow snow festival (Feb / Mar). Block 1 of each is the announcement: start record 11-14 of the
  classroom's block (`s210` 1-3, `s508` = 0, the dates in bytes +28/+29 .. +31/+32), it sets s508 = 1 and warps into
  the classroom again for the lesson. Block 2 (502: 10, 11) is the town's set-up for the festival week (the teacher
  placed, g1242 the week's event: 124 tea party, 127 test of courage, 93 cookoff), from the town's start records
  (mc_ss block 100 rec 53-55, which also need s411 = 0; day1done.sav has s411 set, so the tests poke it). The rest
  starts by talking to the teacher: 500.3 (join the party / be evaluated; the party itself is a scene at a table in
  the plaza with two classmates, s57, s58; the evaluation reads s529, the hosting score of the classmates' visits to
  the student's room, event 401), 501.3 (take part, the rules, bring back the item from the haunted house), 502.12-15
  (the rules, give mushrooms, "let's eat": the feast and its results, the present), 503.3 (the colours collected,
  the ritual, the present). Their own records: s48, s52-s59, s508, s524-s546, s575 (each student's); the town's:
  g1387-1390 (someone took part, read and cleared by the bulletin board's news, event 260), g1242, and for the test
  of courage g1054 / g1070 (mystery fields).
- **Holidays**: 504 New Year's Eve at Lovers Point (map 49, Dec 31 / Jan 1: Principal Sol, block 1 set-up, block 2
  his talk: the countdown, "Happy New Year"); 505 New Year in the dorm (Jan 1-8: set-ups that give each student the
  New Year's things, s266-s477; nothing to share: each console's own run gives its own student them); Christmas is
  mystery 40 (file 110, Dec 22-25: Santa in town on the 23rd, at Lovers Point on the 24th-25th) and Halloween
  mystery 31 (file 101, Oct 29-31): both are mysteries, so "Mysteries together" covers them.

**Built** (only in a session):
- **Go along.** A festival's scene with its teacher (500.3, 501.3, 502.12-15, 503.3, 504.2) starting on one console
  is told to the other (message 43 "start", as a mystery's). The other student, once free, is asked "<name> is at the
  spring tea party! Go along with <name>?" (the festival's name; event 90016 with t155 = 40 + the festival, lines
  460-464 of bank 60; 90020 "Looks like <name> has already moved on.", line 465). Yes: taken there the mysteries'
  way (into the town at the nearest arrival point, or warp_map into the festival's map, then a step beside), and
  that is all: the student talks to the teacher itself, so the game picks its own part from its own record (the
  first talk, an evaluation, the present) and each is judged and rewarded on its own record. A student already in
  the same place within 12 units is not asked. Arriving within 24 units of the other counts as there (a festival's
  tables and stalls push a placed student a few units away).
- **The host's festival.** A festival scene sends no result to the host: g1387-1390, g1242 and the test of courage's
  g1054 / g1070 are the host's alone (coop_sync.cpp), a guest's writes undone when its scene ends.
- **The announcement: left out in a session for now.** Its second half hangs in a session (Open, below), for one
  student as for two, and it came back every time the student walked into class that month (s508 never set), so in
  a session its record does not match (s508 reads 1; "announcement 50001 left out in a session"): the lesson starts
  as usual, together (verified, `--clock 2000-04-20T09:30`: "lesson 1101 held until slot 1's starts too", the guest
  "starts now", both in Mr. Vlad's lesson to its end, `fest_ann3_*`). Each student still hears it alone at home.
  `MQ_COOP_ANNOUNCE=1` (research) plays it, held for both: the record is held like a lesson's (`mq_class_hold` from
  `mq_coop_var`, the record check reading s508: it reads 1 while held), only when the whole record would match (its
  dates and conditions read with the game's getter); while it is held the lessons' records after it are held too.
  The other student arriving starts its announcement at once, and this one is warped in again and starts.
- **A teacher gets a model in town.** In a session the town has three models for people (two students): the
  classmates near the camera had them all, and Miss Strawe (124, put in the plaza by the tea party's set-up) had none,
  so her scene's first command (0x1C, actor_action_wait on her) waited for ever. The town's model manager
  (`mq_town_models`) now gives a person in the camera's box who is not a classmate (number over 100) a model first,
  a classmate giving theirs back for it.
- **A scene's people stay where it puts them.** While a scene runs on a console, the NPC sync no longer applies the
  owner's places there (coop_npc.cpp; before, only in the classroom): the tea party's two guests had been taken
  from the table on the guest's console (RAM: classmate 38 at (18.7, 60.1) instead of (59.1, 75.0)).
- Test driver: `MQ_MYST_GOTO` also takes `p <x> <z>` (a step within the current map, event 90018).

**Verified** (pairs of day1done.sav, Connect now, `MQ_EVENT_FILE` to start a teacher's talk where pressing A by the
teacher did not start it in the tests (the student stood out of reach: Open, below), s411 poked to 0 so the town's
set-ups run; screenshots in the session's `shots/fest_*.png`, outside the repo):
- Tea party (`--clock 2000-05-03T15:30`): the host's 50003 by Miss Strawe; the guest, in the dorm, was asked
  "AAAAAA is at the spring tea party!" / "Go along with AAAAAA?" (`fest_t2_c`, `fest_t2_e`), Yes: into the town and
  beside; the two students see each other at the tea party (`fest_t2_j`, `fest_t3_now`). The guest's own talk ran
  ("Oh, AAAAAA! What can I do for you?", the other student a step beside, `fest_t2_k`), "Join the party": the party
  scene ran to "Thanks to you all, the tea party was just so much fun!" and its end. With the model change Miss
  Strawe got model 3 ("coop town: model 3 asked for person 124") and the scene that had waited for her ran.
- The announcement held for both (`MQ_COOP_ANNOUNCE=1`; `--clock 2000-04-20T09:30`, g1017 = 13 on both): "announcement 50001 held until slot 1's starts
  too" on the host, the guest walking in "starts now (the other student: waiting in class)", the host into the
  classroom again, both "Before class starts, Mr. Vlad has something to tell you youngsters." together, the other
  student at the desk beside.
- New Year (`--clock 2001-01-01T10:00`): the host at Lovers Point (MQ_MYST_GOTO 49), Principal Sol's 50402 ("Happy
  New Year, young AAAAAA!"); the guest asked "AAAAAA is waiting for the New Year at Lovers Point!" (`fest_ny_2`), Yes:
  warp_map 49, "beside the other student at festival scene 50402", both students by the bell (`fest_ny_3`); the
  guest's own talk then offered the host the same.
- Christmas (`--clock 2000-12-23T10:00`, Santa's 11002 queued on the host): the guest was asked, Yes, joined
  ("scene 11002 joined here"), both ran Santa's talk ("Ho ho ho! Have you been naughty or nice?", `fest_xmas_3`).
- QUICK PASS.

**Open** (seen in the tests):
- (Fixed: scenes that set their cameras with command 0x47 showed the sky on the bottom screen in a session; see
  "Scenes' cameras in a session" below.)
- **The announcement's second half hangs in a session**: after "Before class starts, ..." the scene's warp_back
  (command 0x62) reloads the classroom for the festival's teacher, and on both consoles it stayed on a black screen
  (no NPC with a model, models 1-3 back with the player objects, model 4 empty; the room's placement,
  mq_class_summon, not called again). The same with one student alone in a session (the guest had no lesson
  picked, `fest_ann2_alone`): not caused by the hold. Hence left out in a session (above).
- (Resolved, not a bug: the tests' set-up.) Talking to a festival's teacher by pressing A by them did not start the
  talk in the tests: they stood the student by `MQ_POS_FILE`, which in town is the bottom camera's point about 7
  units south of the student, so the student stood out of reach north of Miss Strawe. Alone, by pad, on May 3 with
  s411 = 0, standing 1.2 units north of her facing her, A gives her talk ("Pleased to meet you, ..."), and her menu
  starts the party: block 3 is started by her talk, bank 32 line 30 (`$073'500'03;`; 501.3, 503.3 and 504.2 by
  lines 105, 305, 510). How to reach these talks in a test: docs/TESTING.md, "Talking to a festival's teacher or
  Mr. Graves for real". Talking to her in a session was not run again this way (the NPC talk in a session works
  since the fix in docs/COOP_PLAYTEST.md).
- Not run: the test of courage (501, mystery 37's haunted house), the hotpot cookoff (502, map 66), the rainbow snow
  festival (503), Halloween (mystery 31). They use the same code paths as the tea party, New Year and Christmas.
- The tea party's evaluation reads the hosting score (s529) from classmates visiting the student's room; room visits
  are off in a session, so hosting is done outside a visit. Where (static): the dorm rooms' doors, event 254 blocks
  60-63 (rooms 22-25), after the walk in: "if t212 != 0 go to #70" (the warp into the room) skips the visit's set-up
  (s76 = 0; for the student's own room, s1, queries 0xC4(392), 0xBC(392), 0x5D(82) pick the visitor into s76). Only
  the host has its own room in a session's town (a guest's room is in its own town), and the visitor would need a
  model in a building, which a session does not give (field+0x3EAC is 0 indoors: "Classmates in town", not done
  for buildings), or the visit's scene would wait for them as Miss Strawe's did. Not lifted.
- A guest's festival flags stay in the host's town; the guest's own town does not see that it took part.

### Scenes' cameras in a session (fixed)

The bug: in a session, scenes that aim their cameras with event command 0x47 (camera_set) showed the sky on the bottom
screen: the tea party's table scene (blank, `fest_t2_n`; alone it shows the table and its guests, `fest_solo7`),
Principal Sol's New Year talk (the bell at the bottom edge, `fest_ny_4`), and most mysteries' scenes (0x47 is in most
of files 071-122; 1532 commands in all the event files).

**Cause** (static: the recompiled code; RAM dumps of a session and of single player at the same scene):
- The field has two cameras, k = 0, 1 (field+0xC4 + 0xA0k; its event cameras field+0x24 + 0x50k), and the 3D pass of
  parity p draws through camera p (the view objects 0x020AD7B8[p]). With 3D on both screens camera 0 is the top
  screen's and 1 the bottom's. With 3D on the bottom screen only (a session: the field is loaded with dual 0) there is
  one pass, parity 0, so the bottom screen is drawn through camera 0.
- The field knows: its camera setups (the town ov002 0x020FFA44, an area ov003 0x020FF968, ov001 0x020C5FEC) give
  camera k the CameraParam view of screen `k XOR s`, s = "3D on the bottom screen only because of the session"
  (field+0x3EAF; in the town always, in an area when its flag 4, a top-screen view, is set: the same test as ov001
  0x020D14F8). So in a session camera 0 gets the bottom screen's view (RAM: the session's camera 0 holds the records
  single player's camera 1 holds, pitches -38.95, -63.17, ...; single player's camera 0 -1.26, -26.59, ...).
- The scripts were written for 3D on both screens: a scene sets camera 0 to the top screen's aim and camera 1 to the
  bottom's (the tea party: camera 0 level at the guests' heads, eye (60, 5.7, 82), pitch -7.55; camera 1 down at the
  table, pitch -45.23). The game's own helper for scene cameras, main 0x0201757C, asks ov001 0x020D14F8 and then sets
  the bottom screen's camera 1 as camera 0 and leaves camera 0 out. The event commands 0x47 (ov028 0x0211308E -> ov001
  0x020D14C4), 0x0F camera_eye and 0x10 camera_target (ov028 0x02111F80 / 0x02111FC8 -> ov001 0x020D1408 /
  0x020D1438) call the camera directly with the script's k and never ask (the DS has no story scenes in a session).
  So in a session the bottom screen got the top screen's aim, the upper half of a tall view: the sky. Copying camera
  0's values into camera 1 changed nothing because camera 1 is not drawn then.

**Fix** (`game/src/cfie_hooks.cpp`):
- `mq_scene_camera` (ov001 0x020D14C4, 0x020D1408, 0x020D1438, `instead`, only when called by those commands): when
  ov001 0x020D14F8 says the field has one 3D screen because of the session, camera 1 (the bottom screen's) is set as
  camera 0 and camera 0 (the top screen's, not shown) is left out, as the game's own helper does. Everything else is
  unchanged: single player, and rooms and shops that have one 3D screen alone too (0x020D14F8 is 0 there, and their
  scenes aim camera 0). Command 0x4A (camera_param, 60 uses, both cameras the same values in the files seen) and 0x4B
  (reset: each camera to its own setup) are left as they are. Research: `MQ_SCENE_CAM_LOG=1` (each command: the
  camera, one-screen session or not), `MQ_SCENE_CAM_GAME=1` (as the game has it).
- 3D on both screens in a session (now the default, below): `mq_single_3d_session` (ov001 0x020D14F8, `instead`)
  answers no, and `mq_camera_param` (ov001 0x020D0378, `before`, from the three camera setups) gives camera k screen
  k's view again, as in single player. This is why the earlier `mq_dual_in_session` pointed the bottom screen at the
  sky outdoors: the field drew camera 0, set up with the bottom screen's view, on the top screen, and camera 1, set up
  with the top's, on the bottom.

**Verified** (pairs of day1done.sav, Connect now, `--clock 2000-05-03T15:30`, s411 poked to 0, the host's 50003 by
Miss Strawe queued with `MQ_EVENT_FILE` and "Join the party"; screenshots outside the repo in
`/home/claude/run/live/shots/cam/`):
- Default (3D on the bottom screen only): the table scene on the host's bottom screen shows the table, the guests and
  the student, framed as single player's bottom screen (`a_2_table`, `final_2_table` with `NTR_DRAW_EVERY=0`; before:
  blank, `fest_t2_n`); the log "scene camera: ... camera 0, single-screen session 1" then "camera 1". The field camera
  walking outdoors is unchanged (`a_0_field`). New Year (`--clock 2001-01-01T10:00`, MQ_MYST_GOTO 49, 50402): the bell
  and the student between the pillars (`n_1`; before: the bell at the bottom edge, `fest_ny_4`). QUICK PASS.
- `MQ_COOP_DUAL_3D=1`: without `mq_camera_param` the field camera was wrong as before (the bottom screen the sky,
  `d_0_field`); with it the town is on both screens as in single player on both consoles (the post office, the dorm's
  tree door, Strange House, the bank: `h_0_field`, `h_3_school`, `h_4_out`), and the tea party's table scene has the
  sky on the top screen and the table on the bottom, as in single player (`h_2_table`).
- Not run then: a mystery's own scene (8204 queued alone ended at once: it needs its creature's set-up), the 0x0F /
  0x10 scenes in town (files 074, 093, 103, 212-227, 250).

#### 3D on both screens in a session (the default)

Every field that draws 3D on both screens alone does so in a session too (the town, and the areas with a top-screen
view, area flag 4: the dorm, the school's hall); in the others the top screen is what the field puts there (the
classroom's board in a lesson; a shop like the bank: black, the session's signal icon still drawn). The DS's session pattern and its signal icon are then not shown: the icon is the
game's (a radio's strength, on PCs always full); the window's title already shows the co-op link between PCs
("co-op: hosting / joined, n other PCs"; over the internet the round trip and the stand-in). The setting is read
outside a session only (`cfie::coop_top_3d`, set by the frontend while no session runs), so a field keeps one answer
for the whole visit; a change takes effect from the next visit. The PC view layout is unchanged in a session: its
camera hooks stay off while a co-op link is set up, so it shows the bottom screen alone, as before.

Verified with the switch on (pairs of day1done.sav, Connect now, software renderer; shots outside the repo in
`/home/claude/run/live/shots/d3/`):
- **The town** by day (2000-01-18 10:00: snow, Strange House on both screens of both consoles, `d3h/town1.png`) and
  at night (2000-10-28 3:26: the night sky with its stars above the town on both, `night_0`); walking and warping
  about the town (the general store, the boutique, the flower shop, the tree house: `bld_*`).
- **Buildings with a top-screen view**: the dorm downstairs (the floor above on the top screen, as alone:
  `shots/d3/dorm_down.png`; upstairs by the Gateway, where Connect now leaves both, the top camera looks at nothing:
  black on both consoles), the school's hall (its upper wall and door on the top screen: `bell_1`).
- **A mystery's scene** (mystery 12, its creature's set-up 8232 queued on both, the host's scene 8204: the guest near
  joined it at once; both consoles followed the scene's cameras, the walk to the river and the low view of the
  students and the creature among the mushrooms, the top screen the scene's sky as alone: `m12_*`).
- **A lesson together** (g1017 = 1 on both, the host in first: "held until slot 1's starts too", the guest "starts
  now"; the principal, two classmates, the other student at the desk beside, the lesson's board on the top screen of
  both, to "Well, that should just about...": `shots/d3les_*`).
- **The Gateway's Anywhere** (the host's menu, the walk in, the beach; the guest asked, Yes, beside the host on the
  beach, both screens of both: `d3h/gw_*`). (The host's sky there was the Anywhere day's own, an evening sky with a
  star, and the guest's the day's rain: the test switch `MQ_GATE_ANYWHERE` sets the day's Anywhere flag, field+0x3EB0,
  on the host only; in a game it is the host's town's g1262, which a guest's session start copies too.)
- **The new day's reconnect** (connected at 3:27, at 3:30 "Disconnect?", both home, both connected again into the
  host's dorm by the Gateway, then in town together at night, the town on both screens: `newday_*`).
- The tea party's table scene (earlier, `h_2_table`: the sky on the top screen and the table on the bottom, as alone).
- QUICK PASS.

### The Gateway's destinations together (built)

`game/src/coop_gate.cpp` (side channel message type 46), events 90030-90034 and lines 470-478 of ours
(`game/src/coop_content.cpp`); docs/COOP_GAME.md item 6. What the game does (event 250 block 1, started by touching
the Gateway upstairs in the dorm; static, with the script decoder, the disassembly and solo runs where noted):

- **How a session hides the destinations**: not a menu text tag. #18 asks query 0x22 (t20 = 1 in a session) and #19
  `if t20 != 1 goto #24` falls through to the session's own menu, 999:5315 "communications room / Exit". Alone, #24
  on picks a menu by progress (each with Save, query 0x49: field change 3, save and back to the title):
  - `s411 != 0` (a student the principal has not yet told about the Gateway: set from the start, cleared by his
    talk 250.12/14, which also gives s410; day1done.sav has it set): "communications room / Save / Exit" (5365).
  - mystery 31 at stages 1-2: the room refused (5352).
  - **Anywhere** (5320, #166) when query 0x96 is 1: field+0x3EB0, a flag of the day (the town's g1262, copied to
    0x020AD7F0+14 and the field by the start of day ov001 0x020D5C14 and, in a session, the session's start 0x020F22D8;
    no script writes it). The walk into the Gateway, then query 0x25's t55 picks the place: the outdoor area 64 (the
    beach), map 67 (no door rects: the game itself sends a session to the school's hall 6 instead, its own t20 test
    at #207), the outdoor area 63, the school's hall 6, the haunted house 9 (after query 0x27), the books building 53.
    Seen alone at 21:00 with the flag poked: the beach at night.
  - **Mystery time** (5330, #343) when query 0x45 (ov001 0x020C7954: the week's mystery solved, the next one due in
    /script/start/obj.dat) is set; with the Key of Mystery (250.12/14, s525). Query 0x48 asks the field for change 2;
    the field's end then runs ov001 0x020C7AAC (0x020C9D8E), which starts the town's next mystery from the rotation
    (g1240, obj.dat) and reloads the field. When it is not due and the student's house (query 0x4A) holds the key
    (g1243): "I hear a knock..." (#371).
  - **The other world** (5340, #228) during mystery 32: with g1010 = 0 the warden catches the student (a scene that
    moves the mystery: g1001 += 1, g1010 = 1, ...); then, with item 2106 (query 0x28), the walk in and `warp_map 38`.
    Map 38 has no door: mystery 32's scenes run there (102.2 on arrival, 102.3 the talk that ends with `warp_map 14`,
    back to the dorm), and that is the only way back.
- **In a session now**: whoever touches the Gateway decides. When the game's menu would offer Anywhere or The other
  world, #19 is made to jump to the event's last command (the jump's target rewritten in the loaded commands, after
  checking that each branch's first command is the one the card has), and our menu (90030) offers "communications
  room / <the destination> / Exit". The pick runs 250.1 again, #19 now jumping to the room's branch (#74, the game's
  own session path) or the destination's (#166, #228): the game's own walk into the Gateway, its random pick, its
  warp. No Save (a visit never saves), no Mystery time (below). Otherwise (s411 set, no destination today) the game's
  own session menu, the command written back as the card has it.
- **The other student** is told when the student goes through ("leaving"), and asked once free to move (also while
  standing in a destination of its own) "<name> is going through the Gateway of the Beginning and End! Go with
  <name>?" (90031). Yes: once the first student stands in the destination it sends the place, and the other is taken a
  step beside it with our warp_map (90032, filled in as it runs: events of ours, as the mysteries' joins, since a warp
  made between frames into the town ends the game in a session). No: nothing happens. If the first student never left
  (the other world without its item: "I shouldn't touch it") the other, having said yes, hears "Looks like <name>
  didn't go through the Gateway after all." (90034).
- **Where "there" is.** Two things had to be learned live. (1) A warp writes the field's area byte at once, while the
  old map is still up and the student still at the old place (the first runs sent the Gateway's spot in the dorm as
  the beach's); the new map's load then puts the student at its arrival point, a jump of the position. So a new area
  counts from that jump on, 20 frames later (and after 250.1 has ended). (2) A warp's place is not in the frame of the
  student's position (0x020B8B7C): `warp_map 64 (5, 14)` puts the student at (5, 25.85), the dorm's (15, 1) at
  (14.95, 10.74), the hall's offset another; a partner warped to the first's *position* landed 12 units off, in the
  sea. The place a warp asks for stays at 0x020AD7F0 (x, y, z in 20.12, the map at +12; ov001 0x020D15B4 writes it)
  until the load takes it: caught each frame, it is what is sent for a numbered map (the town: its arrival points are
  picked by the position). The mysteries' joins (coop_myst.cpp) had the same fault; they now convert the position
  with each map's offset ("Mysteries together", "Where beside is").
- **Back**: when the first student leaves the destination (a door to town, or mystery 32's scene to the dorm), a
  partner still standing in it is asked "<name> is heading back. Go back with <name>?" and taken to where the first
  went: into the town at the arrival point nearest to the first (warp_local, 90033), or a numbered map (warp_map).
  From the other world, which has no way out, the partner goes back without a question ("You go back with <name>.").
  Either can also walk out by the destination's own door at any time; that ends the trip for them.
- **The mysteries' question waits.** The other world starts mystery 32's arrival scene (102.2) at once, which
  coop_myst.cpp offered to the partner as "<name> has found something mysterious! Go and help?" while it was already
  on its way (seen: both questions, one after the other, and the Gateway's offer timed out meanwhile). While a student
  goes along (`mq_gate_following`) that question is not asked; once there, near the first, the scene's own start
  record starts it for the partner too and coop_myst counts it as joined.
- **Kept hidden: Mystery time.** It takes no one anywhere: query 0x48 asks the field for change 2, and that field's end
  (ov001 0x020C9D8E -> 0x020C7AAC) starts the town's next mystery from the rotation (g1240, obj.dat) and loads the
  field again. In a session that is the host's town's week moved from either console's Gateway: on a guest the
  mystery's fields are the host's alone (undone), so the guest would be shown a mystery the town does not have; on the
  host the partner's mystery scenes, offers and vetoes would be cut off mid-way by a change of the town's mystery. It
  was not tried (no test save has the Key of Mystery, s525/s410, and a mystery solved). So in a session the Gateway
  offers it to no one; it is the one destination to play alone.
- **The other world for a guest** only once the warden's scene has played (g1010 != 0): that scene moves the host's
  mystery, and a guest's writes of the mystery's fields are the host's alone. On the host the scene plays as alone,
  and no one is asked to come along (no one travels).
- **Map 67**, one of Anywhere's places alone, has no door: the game itself sends a session to the school's hall
  instead (#207). Left as the game has it.

Verified (pairs from day1done.sav, `--clock 2000-01-18T10:00:00`, Connect now, the research binary; s411 poked to 0
before connecting (then, a guest's poke of its own s411 after connecting did not hold: our s411 test read the host's
student's record, fixed since: see "Students' records" above; verified: the host's s411 left at 1, the guest's
`var 411=0` poked after connecting, its Gateway offered "communications room / Anywhere / Exit"); the day's
Anywhere flag set by `MQ_GATE_ANYWHERE=<file>`, the other world by pokes of g1000 = 32, g1010 = 1 and its item by
`MQ_GATE_KEY=<file>`; the Gateway opened with `MQ_EVENT_FILE` = 25001 where the student stood at it; texts and pair
screenshots on both consoles):

- The host's Gateway: our menu "communications room / Anywhere / Exit" (no Save); Anywhere: the game's walk and warp
  (the school's hall once, the beach 64 after); the guest asked "<name> is going through the Gateway...! Go with
  <name>?", Yes: taken to the beach at the host's side (host (5.0, 25.85), guest (6.0, 25.85)), each console showing
  both students there.
- Back: the host walked off the hall into town (the guest asked "Go back with <name>?", Yes: in town at the school's
  arrival point, beside the host) and off the beach into town (the guest at the general store's arrival point, ~5
  units from the host).
- The guest's Gateway: the host asked, said No: the guest went to the beach alone, the host stayed in the dorm.
- The other world (the host): "communications room / The other world / Exit", the walk in, map 38; the guest Yes:
  in map 38 beside the host, mystery 32's arrival scene played on both (coop_myst: "joined"), no second question.
  The host sent back to the dorm (`MQ_MYST_GOTO` = `14 15 1 6`, as 102.3's warp): the guest "You go back with
  <name>." and in the dorm by the Gateway.
- The other world without the item: the host's "I shouldn't touch it."; the guest (Yes) "Looks like <name> didn't go
  through the Gateway after all."
- Exit: the camera back, free to move; communications room: the game's own session path (the room, "Disconnect?",
  No: back in the dorm, still connected).
- QUICK PASS (nothing runs outside a session).

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
  host's weather changed"). Those bytes are not the sky, though: see "The guest's sky" below.

#### The guest's sky (built)

What the sky is made from (static, ov001 and main, then RAM dumps of both consoles):
- ov001 0x020CFBC0 (from the field's time, 0x020C8F5E, every frame) is the twice-a-day weather *roll* (when the hour
  crosses noon: field+0x15238 the half day it last rolled): it writes store indexes 540-569 (five groups of numbers,
  e.g. 100, 106, 100 and 6, 5, 5) from the RNG; it returns at once in a session (field+0x3EAF). The bulletin board
  (ov016) and a few others read them. The sky does not.
- The sky's **kind** (0x020AD2A5; 1 a clear blue sky, 3 rain, 4 snow, ...) is set at a field load by ov001
  0x020CE760 (field, -1) -> 0x020D8BD8: the kind for the time of day (the phase, ov001 0x020C8B34) and the next one
  (0x020AD2AB) come from main 0x0200A178: a calendar file's class for the week (heap, *0x020AD2B4) picks a row of
  the **town's weekly weather table** (town block +0x6D1C, rows of 8 kinds by weekday; *0x020AD2C0 points there),
  except fixed dates and the week's mystery 42 (g1000 = 42: kind 1); then ov001 0x020D8D48 adjusts it for the
  weather incantations in effect (main 0x02044800 on the effects list, store indexes 81-100: Flower rain 29,
  Rainmaking 50 (rain), Rainbow 26), and the day's Anywhere flag (field+0x3EB0, the town's g1262) gives its own
  evening sky. ov001 0x020D8CCC (kind) shows it (0x020AD2A4 the kind before, 0x020AD2A6, and writes g1517 = kind
  and store index 301), and ov001 0x020D8738 sets the sky, the clouds and the rain or snow up. Within a field the
  sky only moves on with the phase (ov001 0x020D8A0C, from the town's and the areas' updates: a new phase runs the
  change from the kind to the next kind, 0x020D88DC).
- So both consoles make the same sky from the same date and time and the host's town (the weekly table and the
  week's mystery are the host's, through the town sync; the incantations go both ways by the game's own two-player
  path, block 4 of 310/313/334 and query 0x0D). What a guest's sky did not follow is what arrives in its town while
  it stands in a field: the host's table, mystery 42 (in a session the day does not turn, so in play this is rare).

**Built** (`sky_follow` in game/src/coop_magic.cpp): on a guest, the week's table (0x60 bytes at +0x6D1C) and
"g1000 is 42" are noted when a field loads; when they differ from that (the host's bytes arrived) and the student has
stood free for half a second, the sky is made again as the load makes it: ov001 0x020CE760 (field, -1), then the
field's vtable +120, the two calls of ov001 0x020CE81C, the entry the scripts' weather queries use (0x0D, 0x9E,
0x9F); 0x020CE81C itself is not a function of the recompiled code. No roll runs: the kind comes from the date and the
town. The store writes of the sky's kind (0x020D8CCC, 0x020D8D48: g1517, index 301) on a guest are no longer sent to
the host as the guest's own actions (coop_sync.cpp, `kSkyKind`; every console works out the same). The weather
incantations need nothing: their own path shows them on both consoles. `MQ_COOP_SKY_GAME=1` (research): as the game
has it.

Verified (pair of day1done.sav, `--clock 2000-05-24T10:00`, a rainy day, both by the dorm's square; shots outside
the repo in `/home/claude/run/live/shots/d3/`): the host's weekly table poked (`MQ_COOP_POKE_FILE`, 0x60 bytes at
0x020B5D3C: first 0, then 1): the guest "the host's weather: the sky made again (kind 3 -> 0)", "(kind 0 -> 1)": its
grey sky and rain turned to a clear blue sky at once, in the same field (`sky_1`: the host still grey, it had not
loaded since); the host, warped once, showed the same blue sky (with the game's rainbow after rain) (`sky_2`); both
kept running and connected. QUICK PASS.
- (research) `MQ_MAGIC_LOG=1` logs each if-variable test of the incantation events (310-361) with t199;
  `MQ_MAGIC_LIFT=<id>,...` lifts more ids' refusals.

### The evaluation and the magic tournament together (built)

`game/src/coop_hall.cpp` (message type 48; hook ov028 0x0211343C, the query command 0x53, `instead`), docs/COOP_GAME.md
item 5. What the game does (static: event files 510, 521, 590 with the script decoder, the start tables, ov028's
queries, ov027's day roll, the ROM's overlay table; checked live where noted):

- **The evaluation is not yearly**: it is Principal Sol's weekly grading. Asked (590.2), he says "come to the
  auditorium" when the student's countdown s196 is 0 (s507 = 1; else "It's still a bit early for that"). Walking into
  the auditorium (map 8; mc_ss block 8 record 5: s507 = 1, g1039 = 255) starts 590.1: Sol, three classmates (t131-t133,
  query 0x0C) and the student on stage; the student is graded on its own record (titles by mysteries, the tournament
  s200, Captain Dot, mischief, love, trends, the town's beauty, outfits, friends, the monsters, insects, fish; the
  festivals s540-s543; the activities s160-s169; then the rank: query 0x86's score in t49, 10/20/30/40/50 give s3 =
  1-5, 1-Star Wizard ... Magnus Wizard, each with its hat), then s196 = 7, s507 = 0, s558 = 0 and back to the school's
  hall (map 6). The day roll takes the days gone by off s196 (ov027 0x0211928C: field 0xC4 of the record, minus the
  days). Nothing in it is the town's: each student's own record. **The year has no end in the game**: the calendar is
  the PC's clock, the New Year is a holiday (events 504, 505: "Festivals and holidays together").
- **The staff roll** plays on every rank-up: query 0xA0 loads overlay 25 (ov025 0x0210F76C, `/title/stfrll_a.txt`)
  and 0xA1 polls it until it is over. Overlays 23-26 share RAM at 0x0210F0C0 (the ROM's overlay table) and in a
  session overlay 23 is the session's wireless code: overlay 25 cannot be loaded in a session (the DS game never
  plays it there).
- **The tournament** (521): the extracurricular teacher, Mr. Graves (actor 125), stands in town from the 21st of each
  month for a student who knows four of the twelve tournament incantations s296-s307 (510.1, s225 = 1). His
  "Extracurricular lesson" (bank 32 line 1672: `$073'510'3'0;`) enters it (510.3: s225 = 2, warp_back); the town's
  record (mc_ss block 100 record 3: s225 = 2, s234 = 1, +7 = 1) starts 521.1, the gathering in town (the teacher,
  the student and three classmates, "let's proceed over to the auditorium", s225 = 3, warp_map 8); the auditorium's
  record 0 (s225 = 3, +7 = 1) starts 521.2: the rules (query 0x15 before them), then three rounds against t131,
  t132, t133. A round: the teacher draws an incantation the student knows (t80 = random 0..11 until the student's
  field for it is 1: s305, s297, s299, s301, s306, s302, s307, s300, s303, s298, s296, s304; t236 its id),
  "<student> versus <classmate>", "Ready, begin!", query 0xB1 opens the magic words (ov009 + ov034); with t237 set
  the words cast set t238 (main 0x02046D84: 1 right, 2 wrong); the classmate's time is drawn by 0xAF from its skill
  (ov028 0x02119D84, t40/t41) and counted down by 0xB0 (t0: 0 not yet, 1 done first, 2 a miss). Whoever is first wins
  the round; a loss ends it (s225 = 4, to the hall); three wins make the champion (s200). The start records' byte +7
  is the field's +0x3EB0, copied at a load from the warp request's +0xE (0x020AD7FE; ov001 0x020C6B98).
  Events 520 and 530 are not the tournament (actor 125's other rounds with s142/s143, no start record).
- **Mr. Graves' town set-up** is 510.1, started by the town's record 52 of mc_ss block 100, which also wants +7 = 1:
  from the 21st (query 0x25: t52 the day) and with four incantations known it puts actor 125 at (62, 78) and the
  blackboard beside him (command 0x03 a = 94 at (60, 77); 510.99 takes both away with op 0x74 a = -94), and on the
  day's first load (s234 0 -> 1) sets s225 = 1. Byte 0x020AD7FE is made at the day's start (ov001 0x020D5C14, not
  in a session: then it stays as it was): 1, unless the student's s411 is set (a student the principal has not told
  about the Gateway yet), so such a student (day1done.sav's) never meets Mr. Graves nor the town's other +7 = 1
  scenes (the monthly lessons 511-517). Tests: poke s411 = 0 and 0x020AD7FE = 1, then enter the town again.
- **Entering is a scene at his place.** 510.3's warp_back loads the town again where the student stands (by
  Mr. Graves, who was talked to), its record starts 521.1 and the set-up's first commands put the student at
  (59.8, 80.5) and the camera with it: a step. Started anywhere else in town (the tests: a warp into the town by the
  dorm with the entry state poked, z 47 -> 80), the camera moved 33 rows at once and the town's row cache (ov002
  0x020FED08: 32 rows at town+0x29588 for the row at +0x295AC) made a copy with a negative length that overwrote RAM
  into DTCM; the game died at its next VBlank ("fatal: no recompiled function at 00000000 (lr 020136ff)" in the
  IRQ), alone the same as in a session (there the other console first lost the session: the host stopped answering
  the wireless). The same code ended Phantasm cast in town ("Incantations in a session"). Fixed for every case
  (`mq_town_rows`, game/src/town_cells.cpp, ov002 0x020FED08 `before`): a camera move of more than 32 rows makes the
  rows again from scratch, as the game does at the town's load. The far start then plays (alone and in a session),
  but its backdrop is wrong (the plaza's ground and the blackboard not drawn, the people sunk to their heads: the
  game's own state for a scene it never starts there); the real way in starts it by Mr. Graves, where it is right.

**Built** (only in a session):
- **Go along.** The evaluation (590.1) and the tournament (521.1 in town, 521.2 in the auditorium) are offered to the
  other student the festivals' way (coop_myst.cpp: "<name> is being evaluated in the auditorium!" / "<name> is
  entering the magic tournament!", "Go along with <name>?", lines 490-491 of bank 60): Yes takes it beside the other
  student, and its own record decides its own part: its own evaluation when it is due (s507), its own entry, or it
  watches. The town's gathering is offered only to a student who can enter this month (s225 1 or 2); the tournament
  itself (to watch) not to one on its way to enter.
- **The classmates in the auditorium** are kept as in the classroom (`mq_class_summon` / `mq_class_model` for area 8:
  the room's free models, two with two players). The tournament's rounds wait on their opponent's moves, so a round's
  classmate without a model is replaced at the rules by one of the scene's classmates who has one (alone, one
  classmate is met twice; in the duel the first round is the other student's).
- **The staff roll** is passed over in a session (query 0xA0 not run; 0xA1 then answers "over" at once): the rank and
  hat are given as usual, "The staff roll will play once you're back home." (event 90041), and it is owed (a marker
  beside the save, `<save>.staffroll`). Home, free to move for three seconds out of any session: event 90040 takes
  the student to the auditorium (the roll draws on the top screen's 2D, which the auditorium has as in the scene; in
  the dorm the field's 3D covered it), 90042 plays it there (the scene's music 71, 0xA0, 0xA1 until over) and goes on
  to the school's hall as the evaluation ends.
- **The duel.** When both students are in the tournament (521.2), the first round is between them:
  - *Meeting at the rules*: each console holds its scene at query 0x15 (the hook sleeps the script's task a frame at
    a time, as the wait command does) at least 4 seconds and while the other student is on its way (asked, joining,
    or in 510 / 521), at most 2 minutes. Both decide the same way from each other's state (twice a second and on
    each change): a duel when both are at the rules (or the other has decided on one) and they know an incantation
    in common.
  - *The opponent is the other student*: t131 = 110 + its slot (the game's actor for another player), so the round's
    moves, effects, names ("AAAAAA versus BBBBBB") and the winner's line act on it; its classmate leaves the stage.
    The other student's packets are shown at the opponent's place (4 units right: its console puts it where this one
    puts its own student), and a step beside elsewhere in the auditorium's scenes.
  - *The same incantation*: the host draws among those both know and sends it; t80 is set to it at the draw's first
    test.
  - *Together*: 0xB1 is held on each console until the other has reached it too; the round's clock starts there.
  - *Each casts its own words*: 0xAF and 0xB0 (the classmate's clock) are not run for the other student; right words
    (t238 = 1) are held (t238 read 0) and their time (frames from 0xB1) goes to the host, which gives the verdict:
    the shorter time (the host on a tie), or the only one after 1.5 seconds. The winner's console then sees t238 = 1
    (its win), the loser's the opponent's t0 = 1 ("the winner is <other>"). A miss (t238 = 2) is the game's own retry.
  - After the round the winner's t131 is its classmate again, back in the row (the round's end walks the opponent
    there and waits for it, which the other student, gone on to its own ending, would never do); rounds two and
    three are against classmates as alone. The loser's tournament ends as alone (to the hall).
- Research switches: `MQ_HALL_LOG=1` (also each query of 510, 521 and 590 in a session, and each hold),
  `MQ_HALL_TEST=<path>` ("words": the words right, t238 = 1; "miss": wrong),
  `MQ_HALL_SCORE=<n>` (the evaluation's score reads n: 10 or more ranks up and plays the staff roll),
  `MQ_HALL_NO_DUEL=1`, `MQ_HALL_NO_WAIT=1`.

**Verified** (pairs of day1done.sav, Connect now; screenshots in `/home/claude/run/live/shots/hall/`, outside the
repo):
- Evaluation (`--clock 2000-01-24T15:30`, s196 = 0, s507 = 1 poked on both): the host warped into the auditorium,
  590.1 started by its record with Principal Sol and two classmates in models 2 and 3; the guest was asked "AAAAAA
  is being evaluated in the auditorium!" / "Go along with AAAAAA?" (`e1_join.png`), Yes: into the auditorium, its
  own 590.1 started there, both evaluated on their own records to the end ("Keep up the good work!", "You've
  improved your skills as a wizard!") and both in the school's hall after it (`e1_end.png`).
- Staff roll: the staff roll's event run in a session on the host ("the staff roll can't play in a session"), the
  session went on; the guest stopped, the host home: "to the auditorium for the staff roll", "plays now", the roll
  on the top screen (`rh_grid.png`), then the student in the school's hall, free, the marker gone (`rh_end.png`).
  Alone with `MQ_HALL_SCORE=12` the evaluation ranked up and played the roll in the scene as the game has it
  (`rs_3.png`: the roll on the top screen, the stage on the bottom).
- Duel (`--clock 2000-01-23T15:30`, a Sunday; the twelve incantations and s225 = 3 poked, 0x020AD7FE = 1 before the
  warp into the auditorium so its record matches, both sent there at once): both held at the rules 240 frames, "duel:
  the first round is against the other student (actor 111 / 110)", the host's draw 7 (s300) on both; on both
  consoles Mr. Graves between the two students facing each other with their wands (`d2_versus.png`), the magic words
  open on both together (`d3_begin.png`). Guest first (host a miss first, then right): "duel verdict: slot 1 wins",
  the host's "And the winner is... <guest>" with the guest at the opponent's place, the guest's "the other student
  won" from its side (`d4_after.png`); the host's tournament ended in the hall, the guest went on to round two
  against a classmate (`d5_next.png`). Again with the host first: "slot 0 wins", the host won rounds two and three
  (round three's classmate without a model replaced: "round 3's classmate 16 has no model here: classmate 52
  instead") to "Well done, students!" and "class dismissed" (`d8_host_end.png`). QUICK PASS.
- The way in through the town (`--clock 2000-01-21T15:30`, four incantations, s411 = 0 and 0x020AD7FE = 1 poked on
  both, then both into town again by arrival point 239 so 510.1 ran; `MQ_HALL_LOG=1`): alone first, by Mr. Graves,
  510.3 -> warp_back -> 521.1 by its record (Mr. Graves, the blackboard, three classmates, "Hello, students.",
  `real_solo_gathering.png`) -> the auditorium -> "Round one...". In a session the host by Mr. Graves the same: its
  gathering on its console (`real_1_host_gathering.png`), the session went on; the guest by him its own 510.3: its
  own gathering, each console showing both students in front of the blackboard (`real_2_both_gathering.png`); both
  through the auditorium's rules to the duel ("duel: the first round is against the other student", the host's draw
  10), the words open on both together (`real_3_duel_words.png`), the guest first: "duel verdict: slot 1 wins", the
  host's tournament ended in the school's hall, the guest on to round two (`real_4_after_duel.png`).
- The far start that froze before (the host's gathering started by its record after a warp to the dorm's arrival
  point, z 47): with `mq_town_rows` "town rows: the camera moved 39 rows at once (row 44 -> 83): every row made
  again", the scene ran and the session stayed (`far_1_host_guarded.png`, the backdrop wrong as said above); the
  guest by Mr. Graves entered and went along, both met at the rules, the duel ran with the host first ("slot 0
  wins"), the host on to round two, the guest's tournament over (`far_2_duel_words.png`, `far_3_after_duel.png`).
  Alone the same start had ended the game ("fatal: no recompiled function at 00000000 (lr 020136ff)"); with the fix
  it plays (`far_solo_backdrop.png`). QUICK PASS.

**Open:**
- (Resolved, not a bug: the tests' set-up.) **Talking to Mr. Graves.** The tests found him and his blackboard not
  drawn and A by him starting nothing: the student stood by `MQ_POS_FILE` (the bottom camera's point in town, about 7
  units south of the student), so it was about 8 units north of him, with him below the bottom screen. Alone, by
  pad, on May 21 (s411 = 0, 0x020AD7FE = 1, four tournament incantations), standing 1 unit north of him facing him:
  his talk ("Nice to meet you, ...", bank 32 line 1614 "Oh, what's going on there?") and his menu Chat /
  Extracurricular lesson / Quit; the lesson ran 510.3, the gathering in town (521.1, "Hello, students.") and the
  auditorium's rules (521.2). How to reach it in a test: docs/TESTING.md, "Talking to a festival's teacher or Mr.
  Graves for real".
- In the tests the words were set by the test switch, not cast with the stylus; the right words cast for real close
  the words' window themselves (the switch leaves it open).
- The partner's own evaluation result is seen only on its own console (each student's text runs on its own); the
  other student stands beside.
- More than two students: the rules follow one other student.

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
- The bank (deposit, withdraw, safe deposit) uses another menu in a session: George's greeting is 32:5806 instead of
  5804/5805, and its `$018'2` (go on two lines) leads to menu 5808 instead of 5807, whose three services go (`$019`)
  to 5860 "you can't use that service while visiting another town". Which greeting is a talk table's choice (not
  found in the scripts; 5804-5806 are in no script): the menu is where it is lifted ("The bank" below).

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

**Talking to anyone within reach** (`game/src/coop_talk.cpp`): the players' talk asks the game's talk-target finder
(ov001 0x020C85E4) whether an NPC would answer before the game does; the finder's "can talk" test (an NPC's vtable
+0x13C -> +76, main 0x0202E588 -> ov001 0x020E6580 / 0x020E6544) gives the NPC the talk's priority at the town's
director (`*0x020FE384 + i * 32 + 0x17AF` = 8, `+0x17B0` = 1), and a second test in the same frame says no, so the
game's own finder answered "no one": in a session no NPC within reach could be talked to (Principal Sol's lesson menu,
Mr. Graves, classmates). Our looks now put those bytes back (`DirectorKeep`). Found in docs/COOP_PLAYTEST.md.

**Shopkeepers across the counter** (`game/src/coop_talk.cpp`, `talk_to_shopkeeper`, in the talk hook): in a session
nobody could talk to a shopkeeper standing behind their counter. Out of one, the shopkeeper walks about (the full
brain) and comes within the talk finder's reach (ov001 0x020C875C: 2.25 units for 126/127/128/129) of a student at the
counter; in a session every NPC gets the online brain, whose think does nothing, so the shopkeeper stays where the
shop placed them, 2.5 units from the counter's front (the finder's own numbers, Harrows: Piquard at z 24, the student
stopped at z 26.5), and pressing A did nothing. Now, in a session only, when the finder finds no one and the other
student is not in front, a shopkeeper (NPC 126-133) that passes the finder's own tests (a trigger, shown, not left
out, level, within 60 degrees, its own "can talk") within its reach plus 0.75 units is talked to with the talk call's
own steps (main 0x02028132-0x0202819A). Seen at Harrows (both floors), Daisy's and The Emporium (and the bank's George).
Still refused: Phantasm (the other summons and transforms are lifted: "Incantations in a session"), the bank's safe
deposit, Download, a second session. (The trash can, the school bell and the bank's deposits are lifted: below.)

**The tatami room** (`mq_coop_gate`, a second pattern): the room off the dorm hall (event 254 block 45: "if 10020 ==
1" (query 0x22, in a session), then "if 10212 != 0", both going to 999:5306 "The door is locked") now opens on its
own days (query 0x25: every third day of the month) and holds 3 students (query 0x26), as alone. Verified: on a guest
connected on the 18th, event 25445 walked it through the door into area 15 (the room with the fireplace, bed and
books), still connected.

**The bank** (`mq_menu_session`, `mq_bank_target` in game/src/cfie_hooks.cpp): menu 5808's *Deposit* and *Withdraw*
keep their own targets (5990, 5993: events 214.2 and 214.3, queries 0x74 and 0x75): the menu reader (main 0x02038938)
reads an option's `$019` target with main 0x02096D10 returning to 0x02038AC4; "5860" read there after the label
Deposit or Withdraw gives 5990 or 5993 instead. What they write was measured (RAM dumps of both consoles before and
after, a guest at the bank, `--clock 2000-05-24T13:00`): a deposit of 1000 moved the guest's money (its own record
+0x6A0) from 3000 to 2000 and its savings (record +0x17E0) from 0 to 1000; the withdrawal of 1000 moved them back;
nothing else changed in the town blocks of either console, the host's own money and savings were unchanged, and the
host's copy of the guest's record followed (the record sync). So both are the student's own business, like a
purchase. (`MQ_COOP_BANK_GAME=1`: as the game has it; `MQ_MENU_LOG=1` logs the menu reader's numbers.)
**The safe deposit stays refused**: its items are kept as counts in the *town* block (+0x8 on, one byte per item:
a guest's Beige Wardrobe, item 1468, put in for the test left its bag and became 1 at +0x196 of its copy of the host's
town, on the guest only), so a guest's would be lost at the end of the visit and would land in the host's town's
safe deposit if sent. Screenshots: `shots/d3/bank_menu.png` (the normal menu for a guest), `bank_dep*`, `bank_w*`.

**The trash can** (`game/src/coop_trash.cpp`, event 90050 and line 500 of ours, message type 49): the list is the
host's town's, one student uses it at a time, the host decides.
- On every console in a session, 225.1's refusal test ("if t212 == 1 go to #3": guests only) is made to end the event
  (its jump to the end, always), and our event 90050 is queued once 225 has ended: the mark `kTrashAsk` asks, a loop
  waits until t195 holds the answer, then the game's own screen (query 0x54), then the mark `kTrashDone`.
- A guest's ask goes to the host, which says yes when no one else uses the trash can and sends its list with it (15
  items of 2 bytes at town +0x644A); the guest writes that list into its copy before the screen opens. When the
  guest's screen closes it sends its list as it now is, and the host writes it into its town; the town sync brings it
  to everyone. The host's own ask is answered on its console the same way. The other student is told "Someone else
  is using the trash can right now." (no answer within 5 s: the same). A use that never ends (a guest gone) is let
  go after five minutes.
- Not covered: event 240.1 (a bare query 0x54, no session test; started by nothing found in the start tables).
  `MQ_COOP_TRASH_GAME=1`: as the game has it.

**The school bell** (coop_trash.cpp too): the "bell" is the Organ Man in the school's map 7 (event 221.12: "if t212
!= 0 go to 32:8614 *twang* You can't use that while...", host and guest alike; then Play music, query 0x58(0), ov013's
`/chime/` screen; Reset, query 0x58(-1), main 0x02010608: the default melody from main 0x020A3458 copied to town block
+0x8FB8, 68 bytes; explanations). The melody is the town's (the school's bell plays it), so: the refusal is let
through, and when a guest's turn at the organ ends, the melody, if it changed, goes to the host, which writes it into
its town; the town sync brings it to everyone (the host's own turn changes its town directly). Mystery 17's step at
the organ (g1000 = 17, g1001 = 1, g1010 = 1: a scene that moves the host's mystery and warps to map 7) stays refused on
a guest. Verified: the host's melody poked to another tune (first bytes `02030405...`, on both consoles by the town
sync), the guest at the organ (map 7, 221.12): "*twang* May I help you?", Reset, "Shall I reset the bell then?" Yes,
"Well then, I shall return this...": the guest "the melody changed: to the host", the host "a guest's melody is the
town's", both consoles' melody the default (`08004002 09000504...`). Play music's screen itself was not driven in the
test (its tune goes the same way). `MQ_COOP_BELL_GAME=1`: as the game has it.

Verified (pair of day1done.sav, `--clock 2000-05-24T13:00`, both upstairs in the host's dorm, 225.1 started with
`MQ_EVENT_FILE=22501` as touching the trash can starts it; RAM dumps of both consoles): the guest asked, the host said
yes, the guest's TRASH CAN screen opened (`shots/d3/trash_1.png`); the host's 22501 meanwhile: "Someone else is using
the trash can right now." (`trash_2`); the guest threw its Beige Wardrobe away and pressed OK (`trash_34`): "slot 1 is
done: its list is the town's", both consoles' lists `bc05 ffff...`, the guest's bag empty, the host's bag its own
wardrobe; the host then opened the trash can, saw the guest's wardrobe in it (`trash_5`) and took it out: both lists
empty, the host's bag two wardrobes, nothing doubled; with the host's screen open, the guest was told "Someone else is
using..." (`trash_7`). QUICK PASS.

**The "storage chest" is the dorm's trash can** (lifted since: "The trash can" above). Event 225 block 1 ("if 10212 == 1, go to 999:1021";
flag byte 212 is 1 on a guest, 2 on the host, so only guests are refused) opens query 0x54, ov017's screen titled
"TRASH CAN" (the overlay's other screen, `/trashbox/chest_bg.bin`, is not opened by it). Lifted for a test, a guest
threw its Beige Wardrobe away: the item left the bag (the student's record +0x636, 15 items) and went into a list in
the *town* block (+0x644A, 15 items), which the guest kept as a change of its own ("kept here"), never sent. A guest
could take the host's thrown-away items back out of its copy (a duplicate), and its own would vanish when the host's
block is sent again. Hence the list made the host's, one student at a time ("The trash can" above).

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
