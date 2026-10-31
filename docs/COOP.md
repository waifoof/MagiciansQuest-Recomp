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
  ov001 0x020EFA86, 0x020EFCB8), a link lost (session event 7 "...disconnected due to a communication error." or 8
  "Unable to receive data.", the field loaded again with change 5: home, no save). Every link of ours reports a
  partner whose game ended or went silent for 8 s to it (docs/COOP_INTERNET.md, "A player's game gone").
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

**When a player has something open at the new day** (fixed after the co-op week playtest, docs/COOP_PLAYTEST.md:
the guest left in an open text box; over the internet its stand-in host's Disconnect met "Unable to disconnect."
after 20 s, the guest stayed in the old session with it while the host, home and connected again, hosted a new one,
and both windows said "playing together"). A console's session does not answer a leaving host while a text box, a
menu or a scene of its own is open. Now:
- The host waits until it is free itself (as before: its player closes what is open), then asks before its
  Disconnect: message 41 with a byte, 1 "the visit ends for the new day: get free" (an earlier build's 41 has no byte
  and is taken the same way). The guest's console closes what it has open, as a player would: a text box is paged
  with B (B pages as A does and picks nothing), a menu (the box's menu +0x2BC is 1 while it shows; B does not close
  the game's menus) goes down to its last option (Never mind, No, Exit; Up once if it wrapped round to the first),
  then A; anything else that keeps the student from moving gets B. Once free for half a second it answers 41 with
  2 "free", and the host's Disconnect goes through. Meanwhile both players' own buttons are left out (a new talk or
  menu would block it again). The host waits at most 30 s for the answer, the guest closes for at most 30 s; then
  each goes on anyway.
- If the visit still splits (over the internet: the host went home while the stand-in host on the guest's PC
  stayed in the session), the guest finds it (`coop_net.cpp`): its host, heard from, has been out of a session for 3
  s after the two were together in this one. Its console then reports itself out of the session (so the host's PC
  shows no "playing together" and its stand-in for the guest leaves the host's new session), shows the stand-in's
  own student again, tells its stand-in host to leave, closes what is open as above (`coop_session::host_gone`), and
  the stand-in's Disconnect goes through: the guest sees the game's own "The person you were playing with has
  disconnected.", "Disconnecting.", ... "Save complete.", goes home and connects again as guest. The window's title
  says "the other player left this visit: joining again" meanwhile. `MQ_COOP_DAY_IGNORE=1` (research, on the guest)
  ignores the host's message, as an earlier build that does not close what is open, to see the split mended.

Verified (`/home/claude/run/live/ndpair.sh`, `ndrun.sh`, `nd.py newday`; the week's saves, clock 3:55:30, spring: the
day turns at 4:00):
- Internet mode (NTR_LAN_DELAY 50:10, 2 % loss), the guest in a text box ("Oh, hi, <name>!", its talk to the host)
  at 4:00: the host "waiting until the guest is free", the guest "closing what is open ... B", "free here", the host's
  Disconnect, the guest "The person you were playing with has disconnected.", "Disconnecting.", ..., "Save
  complete.", both home, both connected again (4:04), both "playing together", both in the host's dorm.
- Internet mode, the guest in its talk menu and ignoring the host (`MQ_COOP_DAY_IGNORE`): the host ended the visit
  after 30 s; the guest found "the far host is no longer in this session", went down its menu to "Never mind"
  ("Maybe later, then."), the stand-in host's Disconnect then went through ("The person you were playing with has
  disconnected.", "Disconnecting.", "Disconnected.", "Saving.", "Save complete."), both connected again and stood in
  the host's dorm on both consoles (`shots/nd/net3split_2_after.png`).
- Same-PC link, the guest in its talk menu and the host in its "came over to talk" box at 4:00: no end while the
  host's box was open; at 4:01 the host's player closed it, the guest went down its menu to "Never mind", the host's
  Disconnect went through on both, both connected again.

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
- The student's position is the field's local player object's place: `*0x020AD7EC + 0x383C`, x, y, z at +0x2C,
  +0x30, +0x34 (20.12 fixed point; x east, z south; per area), read by `mq::student_pos` (`game/src/mq_student.hpp`).
  It is what the talk finder measures from, what the game's movement packets carry, and the frame of the event
  scripts' places. It reads (0, 0, 0) while a map loads (some 100-150 frames after a door or a warp). **0x020B8B7C is
  not the student**: it is the bottom camera's point, south of the student and up by an amount that depends on the
  map (town 3.4-7 units, the dorm 8.22, the beach 11.85, Lovers Point 12.23); positions in this file written before
  2026-10-26 are mostly that camera point (see "The student's real place" below).
- In a session the DS shows a 2D pattern (and a signal icon) on the top screen instead of the town (the 3D on one
  screen only, to keep the frame rate up while the radio runs). **Here the town is on the top screen in a session
  too, by default** (Settings > Network > *Top screen in co-op*: *The town* / *Pattern (DS)*, `online.coop_top_3d`;
  `MQ_COOP_DUAL_3D=0` / `1` overrides it): the field's screen setup (ov001 0x020C6F1C) is asked for both screens in a
  session too (`mq_dual_in_session`) and the session's camera swap is undone with it (see "Scenes' cameras in a
  session", "3D on both screens in a session").
- Test runs side by side: `NTR_LINK_NAME=<word>` gives a set of copies on one PC a shared-memory link of their own.
- Test drivers (not in the repo): `MQ_TEXT_LOG` (each new text box), `MQ_POS_FILE` (the student's place; `MQ_POS_CAMERA=1` the camera's point), `MQ_SCRIPT_TRACE`
  (event commands), `MQ_RAM_DUMP`, `MQ_COOP_SOLO` (the session's rules without a session), `MQ_WARP_FILE` (write
  "map x z" or "e <arrival point>" to the file and the player is sent there; e.g. `e 235` outside the café, `e 243`
  outside the books building, `e 233` outside Harrows), `MQ_EVENT_FILE` (write an event number, file * 100 + block,
  and it is queued; e.g. 90001, the demo of our own content, docs/COOP_RELATIONSHIPS.md), `MQ_COOP_CAST_FILE` (write
  30 or 32: Friendship or Declare love is cast with the other student here, docs/COOP_RELATIONSHIPS.md; any other
  incantation's result id casts it through the game's own cast, main 0x02047A10, "Incantations in a session"; how a
  player casts for real, flag 510 the wizard form: docs/TESTING.md, "Casting an incantation by real input"; e.g.
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
  (So a comparison of the two consoles' blocks also shows these, besides the store's own variables below: after the
  October cookoff of 2026-10-28, +0x3D5..+0x3D7 = g1253, g1254, g1255, the after-class sequence, host 2 3 4 / guest
  0 0 1, each student's own lesson progress; and +0x6948 / +0x694C = classmate 7's (NPC 38) friendship and affection
  toward slot 0, 0x21 on the host, whose student is slot 0 there, 0x20 on the guest, where slot 0 is the guest's own
  student in its own town. Both legitimately per console; nothing to fix.)
- **The classmates' needs** (fixed 2026-10-27, `classmate_needs` in coop_sync.cpp, `mq_npc_decides` in
  coop_npc.cpp). Bytes +0x48..+0x4F of each classmate record are eight values (0-~140; set at a new game from the
  classmate's parameters, ov001 0x020FB5D4 -> 0x020FB654: 8 bytes of a 0xE4-byte table entry by the record's +0x96,
  the NPC id) that the game moves when the classmate does a pastime: the director's round of its people (one NPC a
  frame, ov001 0x020E2A90 -> 0x020EBD4C -> 0x020EBD80) applies the pastime's table entry (director+0x608, 16 bytes)
  through 0x020EBE68 when the NPC's pastime starts or comes round again, and a job's start does it at once
  (0x020EBFC0, from 0x020E1CEC); the values are moved by main 0x0202F388 (classmate, which, 1, change) -> 0x0202F144,
  which also rewrites +0x9F (high bits; the low four are "this player slot met the classmate", main 0x0204C5C8 by the
  local student's slot). Seen with the range watch (`MQ_WATCH=020B5488 MQ_WATCH_LEN=0x854 MQ_WATCH_STRIDE=0xA4
  MQ_WATCH_KEEP=0x48-0x50,0x9F-0xA0`): about every 3,600 frames per classmate doing something, one or two units each,
  e.g. classmate 0 (NPC 72) 56 24 . 32 -> 55 26 . 31 -> 54 28 . 30 ... They are read by the classmates' talk rules
  (ov001 0x020F60A0 compares each of the 8 with a rule's threshold, 0x020F5DEC: which lines and moves a classmate
  picks) and by a field's set-up of its people (ov002 0x020FFD28 -> main 0x02042FE4 -> 0x02043408, per classmate with
  its NPC entry). Only the console that decides a classmate (above: the host, or the guest nearest to it) starts its
  pastimes, so only that console's game moves its needs. **Rule:** a guest's change to them is sent to the host as a
  request (as its student's own) when the guest decides that classmate (by the host's table) or a scene runs on the
  guest (a talk, a gift: the student's doing); when the host or another guest decides it, the host's values go back
  at once (the copy's own start of what the owner already did; at a field's load a guest decides all its people until
  the host's word comes, ov001 0x020E6220); just after a field's load, before any word from the host about this area
  or another (the host's people message now goes also when it has no people, so a guest learns the host stands
  elsewhere, where the guest decides its people), the change waits (not sent, not kept) up to 300 frames, then the
  host's values go back. Before, a guest's changes stayed on the guest only ("changed by itself, kept here") until the
  host's own value changed: the host's game never moved a classmate the guest decided, so the two differed for minutes
  (docs/COOP_PLAYTEST.md, "Soak"), and a talk with that classmate on each console read different needs. Log: `coop
  sync: classmates' needs this console's game moved: sent to the host ...: 0 (id 72)` / `the host's values back
  (another console decides them): 1 (id 6)`; `MQ_COOP_NEEDS_LOG=1` also logs a held change.
- **The classmates' happenings and the talk's turn** (fixed 2026-10-28, the same `classmate_needs`). Bytes
  +0x18..+0x47 of each classmate record are six entries of 8 bytes, two lists of three, newest first (main
  0x02043D44 pushes one into list 0 or 1; an empty entry is FF FF FF FF 10 00 FF FF, main 0x02042F64; valid when
  bytes 0, 2 and 3 are not FF, 0x02042F94). main 0x0202E608 copies an entry, stamps it with the town's hour and
  minute (+0x4 bits 5-9 and 10-15, from +0x2CC/+0x2CD, 0x02043074) and pushes it; it is called by the director of
  the people (ov001 0x020E8B0C) and by event and talk commands (ov001 0x020CB190 / 0x020CB1E8 from main 0x02017768
  / 0x02017874). The talk (main 0x0201557C, the talk object's classmate at +0x1474) picks an entry to talk about
  through main 0x0202E69C, which turns the town's four-bit counter at **town block +0x2BA** (main 0x02044074 ->
  0x02042FB8; the people manager *0x020B8E08, set up by main 0x02042FE4 with +0xC = town +0x2BA, +0x10 = town
  +0x2CC). The field's set-up of its people (main 0x02043408) reads and clears the entries. Like the needs, a guest's
  game writes them for the classmates it decides: before, those writes stayed on the guest for the whole visit (the
  soak of 2026-10-28: classmate 0's newest happening FB 34 81 1E C0 11 41 00, stamped 14:04, and +0x2BA 15 against
  the host's 14, from the first walk to the end of the day), so a talk with that classmate could bring up a
  different happening on each console and the host's town never had it. Now the needs' rule covers +0x18..+0x4F:
  sent to the host when the guest decides the classmate or a scene runs on the guest, else the host's values back.
  +0x2BA (seen moving in the same frame as a new happening of a classmate the guest decided, outside any scene) is
  sent with a classmate's change sent in the same comparison or while a scene runs on the guest, else the host's
  value goes back (`coop sync: the town's talk turn (+0x2ba) this console's game moved: the host's back`). Does it
  matter to play: only what a classmate brings up in a talk (no item, money, relation or save byte of the
  student's), and the guest's copy is thrown away at the visit's end; but the host's town missed what happened near
  the guest. Checked by a poke on the guest (classmate 0's +0x18 and +0x2BA, classmate decided by the host: both
  back at once) and the soak (docs/COOP_PLAYTEST.md, "Soak": day 1, 40 checks, no divergence).
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
  On no the swing ends as the miss; the copy stays as the host has it (it goes with the host's next state when the
  insect is gone there). The host answers in the order asked, so two swings at one insect give it to the first that
  hit. It refuses only an insect no longer free on its console: caught by the host's student, given to a guest who
  asked first, or gone (another insect in the slot); not one whose copy flew off or turned uncatchable meanwhile: the
  guest's net hit the insect as the host's state showed it at most a round trip before, and in single player a net
  that connects catches (fixed 2026-10-27; `MQ_COOP_INS_LOG` names the refusing test).
- **The partner's copy never catches** (fixed 2026-10-27, docs/COOP_PLAYTEST.md "Insects at 200 ms: the butterfly
  lost to both, fixed"). The net's state function tests a swing at something in front (step 10, main 0x0202A00C) only
  for the local student, but a swing in the open (no wall or tree in front, main 0x020297B4: step 30) sweeps with
  the catch test for *any* student (main 0x0202A0AE, all kinds). So the copy of a guest's student on the host's
  console, swinging a moment late, caught the butterfly on the host (+0x13C 0, the copy's +0x5DC the slot) and never
  took it (the copy's branch, main 0x0202A134, only counts the swing out); the guest's own catch was then refused
  ("inactive"), the host's state dropped the insect and nobody had it. The same on a guest's console with the host's
  swing: the guest's copy went inactive and stayed so. Now a partner's copy's catch test runs and is undone
  (`remote_catch`: the insects' +0x13C/+0xCD and the copy's +0x5DC as before; `MQ_COOP_INS_REMOTE_GAME=1`: as the
  game has it). Only the swinging student's console decides. The same class for every other action:
  "The actions of the partner's copy" below.
- **A guest's caught insect while "You caught ...!" is open** (fixed 2026-10-27): the take (main 0x0202B3D4) puts the
  insect object in the student's +0x5A4 and the net draws it in every frame of the catch's steps 60-90 (main
  0x0202A6EC), while +0x5DC is already -1. The copy was deleted as "gone at the host" once kMineFrames (1800) had
  passed: a message left open 30 s ended in a fatal jump through the freed object (no recompiled function at
  e92d4008, lr 0202a6ef). An object the local student holds while its net's steps run (+0xA5 not 0) is not deleted,
  nor the manager reset under it; +0x5A4 is not cleared by the game afterwards, so it counts only then.
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
- **An insect's first move in a session (fixed 2026-10-27, real input)**: an insect's last place (+0x108, where the
  base update's move through the town's collision starts, ov001 0x020DD36E -> ov002 0x02103E30) is 0 until it first
  moves; in a session that first move ended at (0, 0), so a butterfly a student walked up to was gone (never drawn
  or caught again; the same with `MQ_COOP_INS_OFF=1`). In a session an insect whose last place is 0 now gets its place
  there (`mq_ins_update`; `MQ_COOP_INS_PREV_GAME=1` as the game has it). Verified by pad: the host's and the guest's
  catches of a butterfly they walked up to, the other student near (docs/COOP_PLAYTEST.md, 12).
- Open: cicadas on trees often flew off as soon as the students were put beside them (3 of 3 with both students, 1 of
  3 with one; the class think's sensor, 0x020DCFEC, sets "near" from the watched student's distance and height).
  Whether a second student, or the poke (a jump, which the other console shows as a dash), scares them was not
  looked into; walking up was not tried. Looked into 2026-10-27 [S + RAM]: the sensor reads only the distance from
  the insect's place (+0xFC) to the watched student's +0x2C (stored at +0x128, "near" when under the kind's
  threshold) and the height (insect y above a floor and less than 2 units above the student's +0x30), no speed or
  state of the student. The host's copy of the guest's student has the guest's own place and height (RAM: (19.91,
  0.00, 66.10) on both consoles), so a partner scares an insect exactly as a local student at its place would, only
  ~100 ms late at 200 ms; the watch changes over only when the other is nearer by a unit (above). A partner standing
  close is a second student to keep away from, as the game would have it with two students; not changed.

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
  is over 4 units away or it loses interest (+0x1F6). While it is active the other fish stand still.
- **The fight and the catch** [V] (the rod, main 0x0204EE50, its phase +2: 2 the lure in the water, 3 the catch shown;
  the lure is the rod's +0xC): from the bite (state 5, main 0x0204F83C) the rod counts the fight (+8, two a field
  update): below 0x0209DFDA[class] (70-90; the class is the hooked fish's +0x120, at most 4) the fish pulls the lure
  away and each frame with X held there counts (+0xA): at 0x0209DFC0[class] (12-8) the fish gets away ("The fish got
  away", main 0x0204DCD0); after it X reels the lure in (0x0209DFD0[class] a press) until the count passes
  0x0209DFC6[class] (300-350) and starts again from 0; B gives up. Reeled to the student's feet with the fish still on,
  it is caught: the bag (main 0x0204F880: the item from the kind, 0x020D27E8, the size in the high bits), "You caught
  a <fish>!" and its size; when that message closes (phase 3, sub-phase 100, main 0x0204F498) 0x020D2958 moves the
  slot's fish to another free point of its kind (the old point free, the new one taken) and clears the source. An
  escape (main 0x0204DCD0) leaves it where it was with new timers. Kind 66 is caught once: 0x020D27E8 retires every
  fish of that kind. The catch writes only the catcher's record: the bag slot and +0x582 (0 -> 1); the fish book is
  registered separately (8649, "registering in the fish book").
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
- **Giving back.** Once the guest's hook is over (the hooked fish gone and the slot no longer the lure's, nor the
  source of a catch the rod still shows; 30 frames, at least 60 after the grant) the guest sends the slot's record
  back: the host takes the place, the timers and the point (its spawn points: the point the fish had when the host
  gave it away freed and the new one taken, when a catch moved it) and runs it again. The host also says no to a
  guest asking for the slot of the host's own catch while its message shows.
- The catch itself, the bag, the book and the message are the game's own on the console that hooked the fish.
- A new field on the host (a new number, the "epoch") makes every slot the host's again on the guest; a guest whose
  host is not in its field (no record for 90 frames) runs its own fish, as alone; the host takes back a slot when
  the guest sends nothing for 90 frames.
- `MQ_COOP_FISH_OFF=1`: none of this. `MQ_COOP_FISH_LOG=1`: what it does. `MQ_FISH_TRACE=1` (research, also alone):
  the lure, the trigger, the count, the hooked fish's state and place, and the rod's phase, fight count and X count
  whenever they change (every 4 updates while a fish is hooked).

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

**Catches in a session (verified 2026-10-26, pad input only; fixed)**: pair of day1done.sav, `--clock
2000-07-18T14:00`, Connect now, both warped to the bookshop (`e 243`). The driver (`/home/claude/run/live/fishdrv.py`,
outside the repo) watches the fish trace and presses X as a player would: a cast of X held 100 frames, a tap every 24
frames until a fish is hooked, every 12 while it comes, none while it pulls (fight count below 100), taps while it
can be reeled (100-270).
- **The guest** (standing at (40.15, 72.6), facing the river): "our lure found fish 2 (2.63 units): asked the host"
  -> yes -> the bite, two pulls and two reels, "You caught a Colored Carp!" ("Very big") on the guest only
  (`/home/claude/run/live/shots/fish2/g3_caught_pair.png`: the host at the bookshop, the guest's catch). The guest's
  bag +0x638 = item 0xA7FB, +0x582 0 -> 1; the host's record unchanged.
- **The bug it found**: the guest gave fish 2 back as soon as the lure was out, before the catch's message closed,
  so the move (0x020D2958 at the message's end) happened on the guest's copy and the host's record put it back: on
  both consoles fish 2 stayed at point 8 (42.90, 71.11) after the catch. And the host freed no point: its copy had
  already taken the guest's records, so "point 8 -> 8". Fixed (9cb5852): the slot stays the guest's while the rod
  shows the catch; the host frees the point it had when it gave the slot. After the fix: "fish 2 given back (point
  11, at 29.04, 53.73)" -> host "fish 2 is ours again (point 8 -> 11, ...)"; RAM of both: fish 2 at point 11 (29.04,
  53.73), point 8 free and point 11 taken, identical on both consoles.
- **The host** (at (40.73, 92.31), the river to the east): fish 0 hooked by the game's own search, "You caught a
  Loach!" ("Small") on the host only (`shots/fish2/h4_caught_pair.png`); the host's bag +0x638 = 0x27FA, +0x582 0 ->
  1; fish 0 moved from point 0 (45.11, 92.32) to point 2 (27.25, 16.43) on both consoles, point 0 free and 2 taken on
  both; the guest's record unchanged.
- Both consoles stayed connected and free to move.
- **Both students at one fish** (both on the west bank, (40.15, 72.6) and (40.15, 73.93), both drivers at once): the
  guest's lure asked first and got fish 2; the host's lure never counted it (the host's search skips a slot it gave
  away), so only one hook; "You caught a Colored Carp!" on the guest (`shots/fish2/both_caught.png`: the two students
  fishing side by side on the host's screen), fish 2 moved from point 8 to point 9 on both. The "no" answer (a guest
  asking for the slot of the host's hooked fish) was not reached.
- **The catch's panel with 3D on both screens (fixed 2026-10-26, "3D on both screens in a session").** Single player
  shows the FISH panel ("You caught ...") on the top screen and, on the bottom, the student putting the catch in the
  basket (a solo catch: `shots/d3p/solo_caught.png`); with `MQ_COOP_DUAL_3D=0` the panel is on the bottom screen
  (the DS's session layout, `shots/fish2/nodual_caught.png`). With 3D on both screens the panel was right but the
  bottom screen showed only sky (a mushroom floating in it): the catch's session-only field+0x3EB3 = 2 left the
  town's 3D out. Now as alone (`shots/d3p/fix_caught_pair.png`).

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
- **Homeroom's people are the host's** (`mq_class_query`, from the query command's hook `mq_hall_query`, ov028
  0x0211343C). Homeroom takes its people with query 0x0C (ov028 0x021149F0): the town's leaving or new classmate
  (g1062, rolled by the host's day roll, ov027 0x02116124) and the room's picks (t128-t130) become t131-t133, and a
  0 among them is filled by the room's own roll (0x020C3179); the scene then swaps t131 and t133 (551 #20-#23, one
  command a frame) and t133 is the one the story is about. Each console's picks and rolls were its own, so the
  farewell's leaving classmate differed (the week's playtest: Eric on the host, Tony on the guest; here once Marty on
  the host and Eric on the guest, both with g1062 = 72). Now the host runs its scene's first query 0x0C itself and
  sends its t131-t133 (message 42, byte 2: the event file, the three); a guest waits for them (at most 15 s, its
  script's task sleeping a frame at a time, main 0x02003BFC, ARM), puts them in t128-t130 and lets its query run on
  those: nothing is left to roll, and the room keeps and models the same people. The scene's last query (551 #137,
  after the farewell, the room's own) is left as it is. A host's homeroom that began alone long before is taken for
  up to 20 minutes; without one the guest's own people are used ("no people from the host").
- **And t133 stays the story's classmate.** By the script, t133 after the swap is always g1062 (t128 before the
  query), the same on both consoles. But a console whose homeroom began alone (the other student not in the school)
  had t131 and t133 swapped back while the other console's homeroom began during its scene (t128-t130 still 0; seen
  on the guest "68 52 72", then "68 52 68", "72 52 68": the farewell said by Tony, not Eric; the same on the host in
  an earlier run); what does it was not found. `homeroom_keep` puts them back once the scene's own swap was seen
  done ("homeroom's t131 and t133 came back swapped"). The scene's later writes (551 #135-#140, 555 #138-#140)
  fill t128-t130 first and are left alone.
- Research switch: `MQ_CLASS_LOG=1` (every remote packet seen in the classroom; a homeroom's t128-t133 on each change).

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

**The town's own people keep their models (fixed 2026-10-28).** The town's placement at its load (ov001 0x020C6680)
gives each person who is not a classmate a model of its own from the top: first the night's and events' actors
(numbers 181-244, main 0x02027CC0), then the others over 100 (teachers and staff a set-up puts in town), model 4,
then 3, 2, ..., the people's count field+0x3EAC one less each, while it is above 0. Alone the count is 4, so up to
four of them keep models 4..1 and the manager hands out 1..count; in a session it is 1, so one of them takes model 4
and the count reads 0 (the manager hands out none). The manager's last step (4) still runs over every model from
the base to 4, so those people are shown and get their near flag. Our rewrite handed out model 4 whatever the count:
Halloween's Jack-o'-Lantern (actor 211, event 101 block 1 puts it in town for mystery 31, block 2 is its talk "Trick
or treat?") lost its model to a classmate at once, then roamed out of the camera's box and back, asked for one again
(`coop town: model 4 asked for person 211 (not a classmate)`, 16 times in 2 minutes) and gave it back each time:
never drawn, A beside it did nothing. Now (`mq_town_models`): a model above 3 + the count is kept for the town's
own people (not handed out, never given back, still in step (4); logged once: "models 10 kept for the town's own
people (count 0)"), and a person who is not a classmate never gives back a model in step (1) (as alone, where theirs
is the placement's for good). Who this covers: in a session the one such person the placement serves (the actors
181-244 first: the Halloween ghost 211, Santa 220 in town on Dec 23); further ones (a festival's teacher: Miss Strawe
124 at the tea party, Mr. Vlad at the test of courage, Mr. Oakley) get a free model (2 or 3) from the non-classmate
step above and now keep it; the buildings' and Lovers Point's people are `scene_people_models`' (New Year's Eve's
Principal Sol 137, the mail cat 138, the tower's people). **Verified** (pair of the Halloween playtest's saves, host
AAAAAA / guest NORA, same-PC link, `--clock 2000-10-30T03:28:30`, `var 411=0`, the day's turn, the dorm's door in and
out by pad): the ghost holds model 4 on both consoles (0x237c19c, as alone), the log has no more requests for it; the
host set down beside it (`MQ_WARP_FILE` `a 211 1.3`, test driver) and A: "Trick or treat?", the Jack-o'-Lantern drawn
on the host's screen; the guest was asked "AAAAAA has found something mysterious! Go and help AAAAAA?", A, and was
taken there: both screens show it and the scene to its end ("Abracadabra!" ... "Tah-tah!") on both
(`/home/claude/run/live/shots/gh/talk.png`, `joined.png`, `after.png`). QUICK PASS. Santa (Dec 23) not run live.

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
    worked it out." (90020; a festival's "Looks like <name> has already moved on.", the auditorium's below: which one
    by t155, written again before 90020 is queued, since the question's flag bytes are gone by then);
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
- **Where "beside" is in a numbered map.** (Superseded 2026-10-26, "The student's real place" below: what follows
  measured the bottom camera's point, 0x020B8B7C, taken then for the student's position. The student's own place is
  in the scripts' frame, and the offsets now learned are (0, 0); the learning is kept as a check.) A script's place
  (warp_map 0x2D, actor_set_pos 0x02) and the student's position (0x020B8B7C) are in different frames in many numbered maps: the position is the script's place plus an
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

**The town's trigger areas** (worked out 2026-10-26; static, then checked live with a research log of the matcher's
selector): `/map/event_rect/event_rect_comp.bin` (LZ10) has 2 sections; the first (70 entries of (offset, size)) holds
the numbered maps' rects (entry = the map), the second (249 entries) the town's pieces. A rect is 24 bytes: x0, z0,
x1, z1 (20.12, local), then 8 bytes; a trigger area's are `00 00 ff ff 00 00 00 <selector>` (the dorm's exit rect in
entry 14 holds 236, the selector of 001.4). The town uses 30 of the pieces, one per 16 x 16 block from (4, 4): block
k = row * 5 + column (column (x - 4) / 16, row (z - 4) / 16), its piece in the list at the rect manager's +16 (count
+20; RAM 0x022D4580 in the tests), and a rect's world place is the block's corner plus its local coordinates. Some
mystery selectors in town: 10 (41.5-46.5, 34-35, just south of the school's front: 51/3, 16/4, 7/3), 13 (41.5-46.5,
34.5-35.5: 7/7), 11 and 14 (45.5-50.5, 76.5-79.5, west of the plaza: 7/1, 7/5, 16/5, 32/1-3), 27 (58-63, 74-79, the
plaza: 16/1, 23/1), 56 (64-68, 75.5-80: 43/1, 43/3), 12 (8.5-12.5, 47-48.5, on the western cliff: 12/1, 6/1, 7/6), 15
(30-32, 12-16, behind the hedge north of the river: 51/1), 17 (5.5-18.5, 18.5-20: 12/2-8, 20/1). Walking from the
school's arrival point straight down (south) crosses 64, 53, then 57, 13 and 10 (seen in the log, the student's place
from `MQ_POS_FILE`).

**Walking into a mystery's trigger area in a session** (fixed 2026-10-26; before, nothing started). The pair
(day1done.sav copies, `--clock 2000-06-25T10:00`, s411 = 0 on both, g1000 = 51, g1001 = 3, g1010 = 1, g1002 = 0 poked
on the host in town): a student walks south with the pad into selector 10, whose map_u_ss record (block 502) starts
121.4 ("Is this it? Is this what you call school?"). Alone it starts at once; in a session it started nothing, on
any try. What was found:
- A mystery poked after the town has loaded is not in the trigger matcher (the map_u_ss matcher, field+0xA69C, takes
  its block from g1000/g1001; a field load clears an unscheduled g1000, so poking before the load does not work
  either). Any event that ends (here the stage's own set-up block 121.30 queued with `MQ_EVENT_FILE`) makes the
  matcher take the current stage: from then on its records are checked (logged: record 121.4 at selector 10).
- In a session every condition of the record held (ov001 0x020F4AD0), the matcher put the event on the student (ov001
  0x020CE108 -> main 0x0202329C: +0x5E4 file 121, +0x5E3 block 4, +0x5E1 the mystery 51) and started its action 7
  (main 0x0201E4DC). The action's steps (the step at +0xA5; main 0x02022CF0-0x02022EF8): step 9 asks the session
  (main 0x0202BEC8 builds request 55: +4 the file, +6 the block, +7 the kind, the record's +76 that the matcher copied
  to ctx+0x23BC; main 0x0202C4CC sends it and writes 55 into the student's +0x5E0), step 10 waits for the answer
  (+0x5E0: 49 or 50 drop the scene, main 0x02022E96, and the trigger put it on again: action 7 and 0 in turn), step 11
  queues the event (main 0x0201DAF0). The answer 50 ("no") came back each time: `32 00 ff 00 79 00 04 04` (50, slot
  0, the town, event 121 block 4, kind 4).
- Who says no: the host's arbitration, ov001 0x020F0C4C (the only code in ov001 that writes a 50 into a message,
  0x020F0E3E: the request's type less 23 when its handler allows, 50 when not). It is reached two ways: from ov001
  0x020F0E50 for the guests' requests (it loops over slots 1-3 only) and, **for the host's own requests, from overlay
  23** (ov023 0x02110AF8, the session's code; so a search of ov001 alone finds one caller). Request 55's handler,
  ov001 0x020F0904, allows kinds 0, 1 and 3 when no one else plays that event (0x020F0EDC; then it tells the others,
  0x020F0FF0) and refuses kinds 2 and 4 outright. Nearly every story record has +76 = 4 (all of map_u_ss but two, 331
  of 340 in map_ss): the DS plays no walked-into story scene in a session. Start records of map loads (mc_ss) and
  queued events (ours, `MQ_EVENT_FILE`, the joins) do not ask, so they were never refused.
- **The fix** (`mq_myst_walkin`, ov001 0x020F0904 `instead`, game/src/coop_myst.cpp): in a session, a request 55 of
  kind 2 or 4 for a mystery scene (files 071-122, not block 99) or a festival scene (`festival_scene`) is allowed (r0 =
  1, the handler's body skipped: no "someone plays it" record, as each console plays its own copy); the host then
  answers 32 (55 - 23), the student's action goes on to step 11 and queues the scene, and the rest of "Mysteries
  together" (the other told, asked, joining) follows. Every other request gets the game's answer. Research:
  `MQ_MYST_ARB_LOG=1` logs every request the host arbitrates (ov001 0x020F0C4C, `before`, with the caller) and every
  message the session puts out (ov001 0x020F11B8, `before`; that one includes the movement packets every frame).
- Earlier attempts (reverted, before this fix): the same place, 0x020F0904, was tried and seen as "never reached";
  why is not known (here, rebuilt with the hook registered, it is reached for both slots' requests, logged); skipping
  the request in main 0x0202BEC8 and reading 50 as 47 in main 0x0202C5C4 did not help either.

**Verified** (one pair, Connect now, `NTR_DRAW_EVERY=0`, the set-up above, real pad input, harness
`/home/claude/run/live/w6pair.sh`, `v5myst.py`, `w6walk.py`; screenshots in `/home/claude/run/live/shots/v5/`):
- The host walks in (from the school's arrival point 240, the guest in the dorm's square): "scene 12104 (kind 4) asked
  by slot 0: allowed", 121.4 starts on the host (`hw_1_walker_scene`, `hw_3_both_in_scene`: "Is this it? Is this
  what you call school?"); the guest is asked "Go and help AAAAAA?" (`hw_2_other_asked`), Yes: a step beside the host
  (event 90018), and the guest plays its own copy ("joined here"; its step crossed the trigger area too, and that
  request, slot 1, was allowed and counted as joining); `hw_4_both_mid` (the guest in "Huh?! You mean I can go
  inside?", the host's scene over); both "ended here".
- The guest walks in (from 240, the host in the dorm's square): "asked by slot 1: allowed" on the host, 121.4 starts
  on the guest, the host is asked, Yes, steps beside and plays its copy (`gw_4_both_mid`); the guest's result reached
  the host, which kept its own (the same stage had been played on the host in the test before: "the host played this
  stage's scene itself", the rule above; the stage was poked back to 3 for the second test).
- QUICK PASS.

**A joiner's town after the step beside** (fixed 2026-10-27; game/src/coop_myst.cpp, `Stepped`). The student who
joined from the dorm's square (event 90018: actor_set_pos to a step beside the other, at the school) then had a top
screen of sky only, before and after the scene (`/home/claude/run/live/shots/v5/hw_5_after.png`,
`shots/hc/b2before_5_after.png`); the bottom one was right. The cameras were not the cause: both consoles' field
cameras (field+0xC4 + 0xA0k) held the same view for the same place. The joiner's town had built its map pieces for the
school's cells as plain ground (piece 24 where the walker's had 313-322). Why: the town keeps one **district**'s
buildings loaded (town+0x31BC0, 1-5: the school 2, the dorm's square 5); ov002 0x020FFE90 draws a piece of another
district (its record's byte 0) as ground (piece 24 or 3). The district changes only where the student walks through a
district's zone: ov002 0x02100C84 (from the town's update on frames where the camera stays in its cell; only with the
field's own camera, town+0x20 = town+0x164) reads the camera's student's place, a 16-unit block (5 across, from x 4
and z 4) and its quadrant, a byte of the table at town+0x8F0 (0: no change), and ov002 0x02100D20 (town, district)
starts the district's load (vtable +240, ov002 0x02100A00, a task; while one still loads it does nothing). A load picks
its arrival point's district. The step crosses no zone, so the joiner stayed in district 5 at the school; walking a few
cells there did not change it (no zone), a new load of the town did. **The fix**: after a step of ours within the town
(the 0x02 command filled in by `mq_myst_command`), once the student stands still (or after 90 frames), the district
is the last zone on the straight way from where the step started to where it ended (sampled every half unit), set
through ov002 0x02100D20 (asked again each frame until the district is that one; at most 600 frames). Logged:
"coop myst: a step in town (76.5, 42.8) -> (45.0, 34.0): district 5 -> 2". Verified by pad input (pairs of
day1done.sav, Connect now, the set-up above, `w6walk.py`'s steps with this session's copies; shots in
`/home/claude/run/live/shots/hc/`): the host walks in, the guest joins from the dorm's square: the school on both top
screens after the scene (`b2after_5_after.png`; during the scene the guest's close-up has the school behind,
`b2after_3_both_in_scene.png`); the guest walks in, the host joins from the dorm's square: the same
(`final_gw_5_after.png`, `final_gw_6_later.png`). QUICK PASS.

**The week's mystery is the calendar's on the host (fixed 2026-10-28).** The town's load runs ov002 0x020FFD2E ->
ov001 0x020C7DAC, the arrival check of the week's mystery: alone it first drops a mystery that has ended (ov001
0x020C7C7C: past its record's dates, or g1244, the days since it began, 120 or more; 255 means never started, the day
roll adds to it, ov027 0x02118CEE), then, with g1000 = 0, looks the date up in the calendar (/script/obj.dat records)
and, with one due, sets the field's load step +0x3EAE = 12: the field reloads itself with kind 2 (ov001 0x020C9620 ->
0x020D17A4), whose load (0x020C9D86: step 2) starts it, ov001 0x020C7AAC -> 0x020CE830 (g1000 = n, g1242 the record's
week's event, g1243 = 255, g1244 = 0, the stage fields cleared by 0x020CE89C) and places its people. The function
tests the field's session byte (+0x3EAF) at its start (0x020C7DBE) and before the look-up (0x020C7F38) and skips both
in a session: on the DS a visit starts no new mystery. But the town's other load step, ov001 0x020C9DDA (load step 1,
3 or 4, no script running), calls the same drop 0x020C7C7C with no session test: so in a session the host's town
dropped an ended mystery (or one poked with g1244 still 255: the playtest's repro, `store[0] = 0` with lr 0x020CE88F,
caller 0x020C7D95 found with `MQ_VAR_WATCH_STACK`) and never started the next one. Alone the same drop happens, then
the look-up restarts the date's (the field's steps 0 -> 12 -> 2 logged in solo; 0 only in a session). A real session
lost every week's mystery that began during it, with its dated events (Halloween, Santa, the haunted house), and a
host that only played connected never had one.
**Fixed** (`mq_myst_town_arrival`, ov001 0x020C7DAC `instead`, game/src/coop_myst.cpp; a hook, so a regeneration): on
the host's console in a session, for the town's own call (lr 0x020FFD33), the function runs as alone with the field's
session byte cleared for the call ("coop myst: the host's town checked the week's mystery as alone (g1000 a -> b, the
date's mystery starts)"); the reload with kind 2 then runs in the session as alone (no fault seen). A guest's town is
the host's (its call stays the game's: skipped); the town sync brings it the host's g1000 / g1001 / g1244.
**Verified** (pairs of day1done.sav host and second_day1.sav guest, Connect now, binary of fa9463f, s411 = 0 on both,
no mystery poked; `/home/claude/run/live/gk_out.py`: the host warped into the bookshop, `e 244`, then walked out of its
door by the pad): Dec 23 10:00 and Dec 24 21:00 g1000 0 -> 40 on the host and the guest; Jun 25 10:00 -> 51 (the town
under mystery 51's red sky on the host's screen, `shots/g1k/hw_1_walker_scene.png`); Oct 30 10:00 -> 31 (Halloween's
sky in the pair, `shots/g1k/hw_halloween_out.png`; the sweep's pair had a grey sky). Walking out of a building and
later town loads kept it (g1244 0); the playtest's repro (Dec 23, g1000 = 40 poked with g1244 = 255, `e 240`): the drop
clears it, the look-up starts 40 again at once (g1000 40 on both, g1244 0). No fatal. QUICK PASS.
Not reached in these tests (the same in a solo control, so the mystery's state, not the session): Santa at Lovers Point
on Dec 24 21:00 (no actor 220 after a fresh start of mystery 40 on the 24th, also with g1244 = 6 poked; the sweep saw
him with g1000 poked on day1done's fields), the mystery-51 walk-in recipe below (121.4 at selector 10 with g1001 = 3
poked after the calendar's start: nothing starts, solo neither; instead the classmates' "It's mystery time!" walk-in
talk 610.1 started there once in the pair), Halloween's ghost (actor 211) by day at (12, 86).

**Halloween (mystery 31, event file 101) and the day's flag (fixed 2026-10-28).** How it runs alone (decoded with
the script decoder; each step played alone and in a pair by real input): the town's start records (mc_ss block 100,
records 127-132) place things by the stage g1001: 101.1 at stage 0 (and 4) puts the Jack-o'-Lantern (actor 211) in
town with the pumpkins, 101.3 at stage 1 with g1010 = 0, 101.5 at stage 1 with g1010 = 1 and at stage 2, 101.7 at
stage 3. Code starts the talks: A at the ghost at stage 0 runs 101.2 ("Trick or treat?"; Treat with Candy, item 441,
counts g1003 and at the second gives a present, 101.8 when the bag was full; Trick: "Abracadabra!", the student takes
the ghost's looks, g1290 = g1307 = g1070 = 1, **g1001 = 1**, warp_back). The load after it runs 101.3: "You will have
to catch me if you wish to return to normal!", the ghost goes off the map to (120, 0), g1029 = query 0x0B(393 + the
branch picked; 6 in each run), g1010 = 1; 101.5 keeps it there at later loads. Talking to a classmate then (any:
72, 76, 6, 79 tried; they answer the changed student with "TREAT!" or "Trick!" and the talk then runs 101.4) sets **g1001 =
2** and puts the ghost at one of three places near the student: (34.8, 4.5) (by the gravestones and the waterfall) or (65.8, 4.8)
on the raised north side, or (11.6, 82) (actor_op69 on the student chooses, by its place it seems; 101.5 at a later load picks one at random).
A at it then runs 101.6 ("Egad! How did you know it was me?! ... Arbadacarba!"): the student's own looks back, g1029 =
255, **g1001 = 3**, query 0x1F(31) (the mystery's done mark, g1324), warp_back; the load runs 101.7 ("No one has ever
been able to see through any of my transformations... Take this as a sort of reward!": one of items 1492-1497), g1001
= 0 (101.1 again: trick or treat once more). 101.98 (at the day's end) undoes the looks and stage if still changed,
101.99 ends it (Nov 1). The ghost walks about and turns away from a student coming near: by the test driver's set-down
(`a 211 1.0`, Up, A) a talk came at once after a fresh load and then rarely (alone: 1 in 22, 1 in 85, 0 in 160 at
stage 2 with the chase; a pair the same), so this is the game's, not the session's: walk up to it soon after a load.
- **The ghost on the mystery's first day.** The start records' byte +7 is compared with field+0x3EB0 (ov001
  0x020F4B7A), copied at every load from the warp request's +14 (0x020AD7FE), and nearly every dated town record
  wants 0. A mystery's start (the town's kind 2 reload above, ov001 0x020D63E6 -> 0x020D6426: a trip through map 44,
  g1020 = 44 and g1258 = 1 for it, back to the town with the byte = 1) leaves it 1 until the next day's start (ov001
  0x020D5CA0 writes it from g1262, the Anywhere day; the Gateway's query 0x96 reads the same byte). So alone the week's
  mystery places nothing on the day it begins: no ghost on Oct 29 (or after a later start), the ghost from the next
  morning (alone: no ghost after three loads with the byte 1, the ghost at once with it poked to 0). In a session the
  host's console runs the start (fa9463f) and gets 1; a guest never did and kept its own 0, so its copy of the host's
  town ran 101.1 and showed the ghost at (12, 82) on the first day while the host's did not (and A there did nothing).
  **Fixed** (game/src/coop_myst.cpp, `DayFlag`): the host sends its byte (message 6 of type 43, at a change and every
  10 s) and a guest writes it over its own (+14 and field+0x3EB0) while connected, its own back at the session's end
  if the game has not written it meanwhile ("coop myst: the day's flag 0 -> 1 (the host's)"). Both consoles now follow
  the game: no ghost on the first day, the ghost on both the next.
- **The ghost at stage 1.** 101.3 sends the ghost off the map on the host only (the guest plays no 101.3; while
  "Go and help?" was up it also took no places from the host). coop_npc.cpp's owner of a person is the nearest
  student, and the guest's was nearer to (120, 0): the host's copy then followed the guest's, still in town, and the
  ghost walked the host's town at stage 1, where alone there is none (A at it ran 101.3 again, "......"; the playtest's
  "3 talks in 400"). **Fixed** (game/src/coop_npc.cpp): a person the host's console moves more than 2 units in one
  frame (an event's actor_set_pos) stays the host's for 10 s, and a guest takes the owner's places while one of our
  own events (file 900) runs ("coop npc: NPC 211 moved by an event here to (120.00, 0.00): the host's for now").
- **Verified** (pair of day1done.sav, AAAAAA host, and second_day1.sav, NORA guest, same-PC link, Connect now, binary
  of the two fixes, `--clock 2000-10-30T10:00`, s411 = 0 on both; harness `/home/claude/run/hw2/` `pair.sh`, `run2.py`,
  `run3.py`, `lib2.py`): the dorm's door (in and out by the pad) starts mystery 31; both consoles' byte 1, no ghost on
  either (before: the guest's only). The next day stood in for by poking the host's byte to 0 (the guest followed):
  the ghost on both. Host: A at the ghost, Trick: stage 1 on both, the "catch me" scene, the ghost at (120, 0) on both
  for good (before: in town). The guest spoke to classmate 79: stage 2 on both. The host's next load put the ghost at
  (72.5, 24.1), A at it: "Egad!", stage 3, the reward scene, stage 0 on both. Earlier in the same work with the host
  leading 1 -> 2 (classmate 6, "TREAT!"). No fatal; QUICK PASS.

**Mystery 39 (the fairy, event file 109) and counting steps (fixed 2026-10-31).** How it runs alone (decoded, then
each step played alone by real input): text messages start events with the tag `$073'<file>'<block>;` (Miss Strawe's
request ends with `$073'109'1`, the classmates' answers with `$073'109'2`, Halloween's "TREAT!" with `$073'101'4`), so
blocks that no start record names are started by talks. Miss Strawe (124, the staff room, map 2, on the days event 221
puts her there) asks at stage 0 (message 1471, 109.1: g1070 = 1, g1191 = 2; her message is 1470 + g1191, and the town's
load at stage 0, 109.6, puts g1191 back to 1). A classmate of the doubting kind (banks 42/43; e.g. 68) asks "Do you
agree with me?" / "...they don't exist, right?" (No) or "Fairies do exist, don't they?" (Yes): each answer that
fairies exist runs 109.2 (g1010 += 1; one per classmate a day); at 5, stage 1. Then a White May Lily (item 207) put on
the ground in Fungi Forest (dragged from the bag to the ground in front of the student, action 26; the item number is
written to t164/t165 as 2/7 by main 0x0202D550, and act_u record 380 wants map 66, record +47) runs 109.3 (stage 2,
g1011 = 1). Fungi Forest's load at night at stage 2 (mc_ss block 66 record 6; at 14:00 nothing, at 19:20 it ran) runs
109.5: the fairy, a reward (query 0xBA), stage 3. Found in a session:
- **Miss Strawe had no model** in the staff room (the visitors' call gave 123 model 4, her none): not drawn, A by her
  did nothing. `scene_people_models` (game/src/coop_class.cpp) now serves map 2 as Lovers Point.
- **Variable-only scenes were offered**: 109.1/109.2/109.3/109.6 are only set_town / add_town / if / goto / end; told as
  scenes, the other student was asked "Go and help?" for nothing to see. Now such a scene (`just_writes`, the commands
  read from the event's start, jumps followed by command index) is not told; nor the Matchstick Girl's purchases
  (118.2-4, `own_business`): told, a student standing near was put into the other's purchase at once.
- **A guest's own short scene left no result**: a short event runs and ends within the frame it starts, before the
  frame's end notices it, and add_town (main 0x0201E1C4) writes the store itself, not through main 0x0201E070, so the
  result said "(nothing)". The town still got the guest's writes, by the game's own way: in a session the interpreter
  sends another console's store commands to the host, which runs them for it (main 0x0201C770; in the pairs' logs the
  host's g1002 went 0 -> 1 from the guest's purchase, and g1001 = 1, before the result came). Now the result is
  complete anyway, as a check and for scenes whose commands do not go that way: a `before` hook on 0x0201E1C4
  (`mq_var_add`, cfie_hooks.cpp) hands the sum to the mystery notes as a write, writes made before the scene is noticed
  are kept (`early`), and an event that ran and ended unseen sends its result ("scene 10902 ran within a frame here").
- **A result over values the host already has**: the result now carries each variable's value before the guest's scene
  wrote it, and the host writes it only where its town still has that value (one already changed, by that way or by
  its own run, is left: "g1002 0 -> 1 left: here 1"; written again, a test build counted a purchase twice), with no
  "the host played this stage's scene itself" for such results (older builds' results keep it).
- **The matchers kept the old stage**: the walk-in and actor matchers are set to the stage only when an event ends
  (main 0x0201C976 -> ov001 0x020CE4E4, 0x020CE530). A stage that arrives from the other console (the host's town, a
  guest's result on the host) left them at the old one: the lily (act_u, stage 1) did nothing (seen alone with the stage
  poked). Now when the stage moves with no event, they are set as an event's end does ("the stage moved here with no
  event ... matchers set to it").
- A ground-object packet (0x1D) of the other student's copy while that copy has no model ended the host once more
  (`fatal: no recompiled function at 00000000 (lr 02031c7f)`, the guest set down beside classmates one after another):
  `mq_coop_remote` (coop_talk.cpp) now passes such a packet over ("its packet 0x1D passed over", seen twice since).
- **Verified** (pairs of day1done.sav, AAAAAA host, and second_day1.sav, NORA guest, same-PC link, docs/COOP_PLAYTEST.md
  2026-10-31): Strawe drawn and her request on the host, not offered to the guest, g1070 on both; persuasions (109.2
  queued with `MQ_EVENT_FILE` as the classmates' text tag does; the classmates' own question by real input only alone)
  alternating guest/host 0 -> 5 on both, stage 1 by the guest's fifth, none offered; the guest's lily in Fungi Forest: stage 2 on both; at night
  (both saved by the host's Disconnect, restarted at 19:20) the host's load of Fungi Forest summoned the fairy, the guest
  was asked "Go and help?" and walked in itself: both saw the fairy (`/home/claude/run/m39/pn5.png`), stage 3, each its
  reward. The Matchstick Girl (mystery 48, Dec 12-14, restarts after the host's Disconnect): the guest's nine purchases
  by real input moved her stages 0 -> 3 on both, the host not pulled in or asked, g1322 = 1 on both; on Dec 23 with no
  pokes Santa stood in town on both and took the guest's match (g1010 = 2 on both). QUICK PASS.

Open: a joining student whose own fields differ (an incantation not cast) may take another branch of
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
- **The cookoff's mushrooms** (502, verified 2026-10-28 solo and in a pair, docs/COOP_PLAYTEST.md "The cookoff's
  hand-over"). "I brought something back" opens the bag in its give mode (query 0x03: ov001 0x020CA530 with mode 1,
  the bag's overlay ov036 keeps the mode at bag+0x7F5; the same mode as giving a classmate an item). A mushroom is
  handed over by **touching it once** (that only selects it: the cursor moves to it and its name shows), **then
  pressing on it again and dragging it out of the bag onto the ground in front of the student** (the field near
  (170, 105)): it is held out and Oakley takes it ("Hmm, that's what you want to put into the pot?" / "All right then,
  just keep bringing me back those mushrooms!"; a Fly Agaric gets "Have you lost your mind?!" and goes back to the
  bag). Seen not to hand over: a press-and-drag that starts on a slot other than the selected one (it only selects),
  and a drop onto Oakley or the cauldron above the student (the item stays in its slot, nothing said). One mushroom
  per talk. The script keeps a running value (s54, compared with 50 and 20 after each mushroom) and the feast grades
  on s52 (compared with 9) and s53 (0 / 1 / 2): two common mushrooms each gave the lowest result ("Um... The hotpot had
  nowhere...", the same lines as nothing handed in). Each console's feast is its own (50213 / 50214 on each, the
  other "already beside"), so each student hands in and is judged alone; no co-op code was needed.
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
- **Both talk to the teacher at once** (fixed 2026-10-26, `mq_myst_walkin`): a teacher's talk starts its scene with a
  request 55 of kind 0 (Miss Strawe's 500.3: `37 01 ff 00 f4 01 03 00`), which the host's arbitration (ov001 0x020F0904)
  allows only while no other console plays that event (ov001 0x020F0EDC with r1 = 2: 0 when it is taken). So the
  second student talking to Miss Strawe while the first one's party ran was refused (`32 01 ...` back, five tries),
  and its talk ended after "You're always working so hard," without the party. Now, for the scenes above, the
  mysteries' scenes and Mr. Graves' 510.3, a kind 0/1/3 request for an event another console holds is allowed without
  being recorded (each console plays its own copy); a free event still gets the game's own handling.
- **A festival's partner asked from 12-24 units away was not moved** (fixed 2026-10-26): it was asked (more than
  kNear = 12 units off) but on Yes the join counted it as "beside" at once, since a festival's wider 2 * kNear (meant
  for a placed student pushed off by the tables) was applied before any move (seen: the guest by the post office,
  13 units from Miss Strawe, "beside the other student" logged, nothing done). The wider distance now counts only
  after a move; otherwise the join steps the student beside (event 90018).

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
  talk in the tests: they stood the student by `MQ_POS_FILE`, which in town was then the bottom camera's point about 7
  units south of the student, so the student stood out of reach north of Miss Strawe. Alone, by pad, on May 3 with
  s411 = 0, standing 1.2 units north of her facing her, A gives her talk ("Pleased to meet you, ..."), and her menu
  starts the party: block 3 is started by her talk, bank 32 line 30 (`$073'500'03;`; 501.3, 503.3 and 504.2 by
  lines 105, 305, 510). How to reach these talks in a test: docs/TESTING.md, "Talking to a festival's teacher or
  Mr. Graves for real". Talking to her in a session was not run again this way (the NPC talk in a session works
  since the fix in docs/COOP_PLAYTEST.md).
- **Real pad input** (2026-10-26, pair of day1done.sav copies, Connect now, `--clock 2000-05-03T15:30`, `var 411=0` on
  both, both into town by `e 239`; harness `/home/claude/run/live/w6pair.sh` + `tp7lib.py`, screenshots
  `/home/claude/run/live/shots/tp7/`, outside the repo): the host walked to Miss Strawe by the pad, A: "Pleased to meet
  you, ...", the party scene 50003 started by her talk (request 55 kind 0, allowed); the guest, at the post office, was
  asked "AAAAAA is at the spring tea party!" / "Go along with AAAAAA?", Yes: a step beside the host (`run2/
  r2_2_guest_moved.png`, after the fix above; before it nothing moved, `run1/t6_guest_arrived.png`); the guest then
  talked to Miss Strawe by the pad while the host's scene was still at her menu: "scene 50003 (kind 0) asked by slot
  1: allowed" and its own party at the table (`run3/r3_1_both_in_party.png`, `run3/r3_2_guest_party_end.png`: the
  guest at the table, "Oh, I'm pretty full right now.", the host still at "What can I do for you?"); both scenes
  ended ("Well, let's make sure we do this", `run3/r3_3_both_ended.png`). Before the fix the same talk got "no"
  (`run2/r2_4_guest_menu_on.png`: the guest's talk closed, no party). QUICK PASS.
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

**New Year's Eve: Principal Sol had no model in a session (fixed 2026-10-28).** 504.1 (Dec 31 from 22:00, the map
load of Lovers Point, map 49) puts Principal Sol (field person 137) at the bell (7, 0.6); a building's placement gives
no one a model in a session, so he stood there undrawn and A at the bell did nothing (his talk, 504.2, is started by
talking to him). Now `scene_people_models` (game/src/coop_class.cpp) gives the people standing at Lovers Point free
models, once the other players' slots are known, as the dorm's mail cat ("coop class: person 137 in area 49 gets model
3"; person 136 too). **Verified** (pair at `2000-12-31T22:00`, s411 = 0 on both, the host taken there with
`MQ_MYST_GOTO 49 7 4 0`, `gk_ny.py`): Sol drawn at the bell on both consoles (`shots/g1k/ny_6_after.png`,
`ny_9_guest_after.png`); the host walked up, A: "Ah!" / "So, young ..." / "Looking back at it, all kinds of things
happened, don't you think?" (504.2); the guest was asked "Go along with AAAAAA?", Yes: beside the host; the guest walked
up to Sol, A: "Ah! If it isn't NORA!" (`ny_8_guest_mid.png`), "scene 50402 ... started here" and "ended here" on the
guest. QUICK PASS.

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
classroom's board in a lesson; a shop, a dorm room, the auditorium: black as alone, with the session's signal icon in
its corner; "The top screen place by place" below). In the fields with 3D on both screens the DS's session pattern
and its signal icon are not shown: the icon is the
game's (a radio's strength, on PCs always full); the window's title already shows the co-op link between PCs
("co-op: hosting / joined, n other PCs"; over the internet the round trip and the stand-in). The setting is read
outside a session only (`cfie::coop_top_3d`, set by the frontend while no session runs), so a field keeps one answer
for the whole visit; a change takes effect from the next visit. The PC view layout is unchanged in a session: its
camera hooks stay off while a co-op link is set up, so it shows the bottom screen alone, as before.

Menus on the bottom screen (`mq_dual_obj_mode`, called from `mq_frame_end`): with 3D on both screens the main engine
draws both screens in turn and the sub engine only shows the last frame's capture (12 bitmap OBJs), so the game sends
the bottom screen's uploads for engine 1 to engine 0 on the frames that draw the bottom screen (main 0x02005970, byte
0x020ACA54, parity 0x020ACA55). A menu label's offset in OBJ VRAM is its character name times the OBJ boundary of its
engine's OAM manager (main 0x0200A970; managers at *0x020AD3A8 + engine * 0x524, the DISPCNT bits 0x00300010 at
+0x520). In single player both are 64 bytes; in a session the communications room leaves engine 1's at 128, so every
label went to twice its offset and the bottom screen kept the room's Yes / No / friend code / Exit (any menu: the talk
menu, the Gateway's). At the end of each frame in a session with 3D on both screens, engine 1's manager gets the main
engine's boundary; when 3D on both screens ends, the game's own value is put back (unless the game has set another).
`MQ_DUAL_LOG=1` logs each change.

The bottom screen's own 2D with 3D on both screens (`mq_dual_field_frame`, from `mq_frame_end`; only for the field
that `mq_dual_in_session` gave 3D on both screens; a field left with 3D on the bottom screen keeps the game's ways).
Two of the field's session-only choices assume the DS's session layout (3D on the bottom screen only, the main engine
there):
- **The field's labels** (the bag's item name, a talk menu's title such as Principal Sol's "Introduction"): four
  labels (OBJ characters 0x160-0x1C0) of a label manager in the field's holder at field+0x204 (class 0x020A3738, set
  up from ov001 0x020C725C through main 0x0201AD18 -> 0x0200EB08). The holder's +4 (field+0x208) is the field's
  session flag (+0x3EAF) and becomes the manager's +0x60, so each label is set up with engine 0 (label +0x241, main
  0x0200A4E4) instead of single player's 1 (measured: solo `eng 1`, session `eng 0`). With 3D on both screens an
  engine 0 copy (0x0200A970 -> main 0x02005970) and its OBJs (0x0200A44C) reach the top screen's frames only: the red
  "Beige Wardrobe" / blue "Introduction" over the top screen, their white bars on the bottom empty. Each frame the
  field's labels get engine 1 again (same characters as alone; the engine is read at every copy and OBJ).
- **A catch's panel**: the fish catch sets field+0x3EB3 = 2 in a session only (main 0x0204EB26; cleared by the game at
  main 0x0204DD8A), with which the town's draw (ov002 0x02101724) and the areas' leave their 3D out (on the DS the
  panel, drawn by the main engine's 2D, covers the one 3D screen). The panel itself (main 0x02013578, engine 0 always)
  is on the top screen as in single player; the 2 is taken back so the bottom screen shows the student putting the
  catch away. (Not changed: the catch code also aims camera 0, field+0x1C, only outside a session, main 0x0204DAD2 /
  0x0204DC9C / 0x0204F738; with 3D on both screens camera 1 is the one aimed, the top one keeps the town's view.)
`MQ_DUAL_LOG=1` logs both ("the field's label n -> the sub engine", "the field's 3D kept on under a catch's panel").
Verified with real pad input in a connected pair (day1done.sav, Connect now; shots in
`/home/claude/run/live/shots/d3p/`): the bag's item name on the host and on the guest (`fix_bag.png`,
`fin_bag_pair.png`, `fin_bag_guest.png`; single player `solo_bag.png`; before `bag1.png`), Principal Sol's menu title
(`fix_sol.png`), a catch by the host (`fix_caught_pair.png`; before `sess_caught.png`; single player
`solo_caught.png`). The other panels were already right (docs/COOP_PLAYTEST.md). QUICK PASS.

**The guest's hair colour on the host's screen** (fixed 2026-10-27; `mq_dual_vram_queue`, game/src/cfie_hooks.cpp).
With two different students (host AAAAAA from `day1done.sav`, orange hair; guest NORA from `second_day1.sav`, green
hair) the host drew NORA with orange hair in the dorm after Connect now, until the first map load; with
`MQ_COOP_DUAL_3D=0` she was green, and the guest's own screen was right. Found with a research build (a dump of the
3D engine's textured polygons and their palettes from VRAM, and logs of the game's VRAM uploads):
- Each player's model has its own textures and palettes in VRAM (slot 0 from palette 0x000, slot 1 from 0x200; the
  hair is an 8x8 texture with palette +0xC0, three colours). On the host slot 1's +0xC0 (0x2C0) held AAAAAA's orange
  (`0d7b 18f7 1d32`), the rest NORA's; her green (`1584 1e06 3668`) was in RAM, in her model's file.
- Uploads go through the game's VRAM queue (*0x020AD6D0: 128 entries of 16 bytes from +4: source, VRAM offset, size,
  kind bytes; count +0x804, the sizes' sum +0x808). Main 0x02006824 queues one (it returns 0 and the upload is lost
  when the queue is full); the main loop does them at the frame's end (main 0x0200686C from 0x02002A78; the source is
  read then) but leaves that out while a session waits (0x0201130C 0, byte 0x020AD7E3 set, session+0xD1 0): 340
  frames on the host while the guest arrives. The wait runs neither the main loop's frame setup nor the frame's end
  (`mq_frame_end`), but it updates the remote players (main 0x02031404) 4-5 times a frame, and with 3D on both screens
  the bottom screen's 2D is queued as often (sources 0x023B364C and 0x023C05A0, about 12 entries a frame). The queue
  was full within a dozen frames. At the end of the wait the host dresses NORA's model in her look (her hair palette
  queued from her file, frame 6607 in the log, with the count at 128: lost; the rest queued after the queue was done
  again went through).
- **The fix**: in a session with 3D on both screens, an entry that is queued again later (the same 16 bytes: source,
  offset, size, kind) is taken out of the queue, from `mq_on_dual_screen_3d` (the main loop's frame setup for 3D on
  both screens) and from `mq_coop_remote` (each remote player's update, which the wait runs). The later entry copies
  the same source to the same place when the queue is done, so VRAM ends the same; the others keep their order. The
  queue stays at a few entries (`MQ_DUAL_LOG=1`: "the VRAM upload queue n -> m entries" when it held 16 or more).
- Verified (pair of day1done.sav / second_day1.sav, Connect now, `--clock 2000-05-21T15:30`, `NTR_DRAW_EVERY=-1`;
  shots in `/home/claude/run/live/shots/hc/`): NORA green on the host's screen in the dorm after connecting, AAAAAA
  orange on the guest's (`final_dorm.png`, `fix3_dorm_apart.png`; before: `/home/claude/run/live/shots/inet/
  samepc_dorm.png` and `shots/hc/fix1_dorm.png`), and in town after the map load (`final_town.png`).
  Every palette of each student's model (hair, face, uniform, skin; 7-8 per student) compared between the two
  consoles from the 3D engine's VRAM: the same on both, in the dorm and in town (the saves have no hats). QUICK PASS.

Verified with the switch on (pairs of day1done.sav, Connect now, software renderer; shots outside the repo in
`/home/claude/run/live/shots/d3/`):
- **The town** by day (2000-01-18 10:00: snow, Strange House on both screens of both consoles, `d3h/town1.png`) and
  at night (2000-10-28 3:26: the night sky with its stars above the town on both, `night_0`); walking and warping
  about the town (the general store, the boutique, the flower shop, the tree house: `bld_*`).
- **Buildings with a top-screen view**: the dorm downstairs (the floor above on the top screen, as alone:
  `shots/d3/dorm_down.png`; upstairs by the Gateway, where Connect now leaves both, the top camera looked at nothing:
  black on both consoles, as alone; since 2026-10-27 the corridor's wall and the Gateway, "The top screen place by
  place" below), the school's hall (its upper wall and door on the top screen: `bell_1`).
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

#### The top screen place by place (2026-10-27)

**The dorm upstairs (fixed).** In a session with the town on the top screen, both consoles' top screens were black
upstairs in the dorm right after connecting (`/home/claude/run/live/shots/hc/final_dorm.png`). Cause: the game's own
top camera, not the session. The dorm (area 14) is one map of two floors (the lobby, floor y 0; the corridor by the
Gateway, y 6) with a top-screen view: camera 0 (field+0xC4) follows the student 9.44 units above its feet, nearly
level (pitch -6.18), camera 1 5.15 above, pitched -24.43 (the height and pitch at camera +0x4C / +0x50, three copies
+0x5C, +0x6C; RAM dumps of a pair with one student on each floor). From the lobby the top screen shows the corridor
upstairs; from upstairs it looks one floor higher, above the model's walls, at the black rear plane. Single player
shows the same black (`/home/claude/run/live/tss/solo4.png`), but in a session every
visit starts there (Connect now, the new day's reconnect, the way back from the Gateway). **Fix**
(`mq_dual_dorm_top`, game/src/cfie_hooks.cpp, from `mq_frame_end`): in a session with 3D on both screens, while the
student stands upstairs (y above half a floor) in the dorm and no field change runs, camera 0's height is one floor
lower (9.44 - 6), so the top screen shows the corridor's wall, its doors and the Gateway above the bottom screen's view
(from the height the top camera has above that floor when the student is in the lobby; nearer, since the student
stands by the wall); the game's values are put back downstairs and when the dorm is left (only if still ours).
Single player and the DS pattern setting are unchanged. `MQ_DUAL_LOG=1`: "the dorm's top camera one floor lower
upstairs" / "back to the game's". Verified by real input in a connected pair (day1done.sav host, second_day1.sav
guest, Connect now; shots in `/home/claude/run/live/shots/top/`): both consoles right after connecting
(`final_connected.png`, `d1_connected.png`), the corridor's two ends (`d1_up_we.png`), the host out to town, in by
the dorm's door (the lobby as before) and up the stairs (`d1_host_lobby_up.png`; the log's back / lower lines).
QUICK PASS. (A wider view upstairs would need the camera's distance, not found: camera +0x54 is not it.)

**Survey** (the same pair with that build, both consoles, each place reached by a warp from a field with 3D on both
screens, as by walking; single player from a solo copy of day1done.sav; shots `shots/top/s3_*.png` and the grids
`s3_gridA.png`, `s3_gridB.png`, `s3_gridC.png`, single player `solo_*.png`, `solo_grid.png`). Bottom screens were
right everywhere on both consoles.

| Place | Top screen, host and guest | Single player | Right? |
|---|---|---|---|
| Dorm upstairs (corridor, Gateway) | the corridor's wall and the Gateway (was black) | black | right (better than alone) |
| Dorm lobby | the corridor upstairs, the other student there | same | right |
| Dorm room (map 22) | black, signal icon (now the DS's pattern, below) | black | right (as alone) |
| School hall | its upper floor and Sol's door | same | right |
| Classroom | the board (2D), signal icon (`s2_classroom.png`) | the board | right |
| Auditorium (8), Sol's office (7) | black, signal icon (now the DS's pattern, below) | black | right (as alone) |
| Town (school, dorm, café, bank, Mon Paris, Harrows fronts) | the town | the town | right |
| Harrows (52) | its balcony upstairs (3D) | same | right |
| Café (57), Mon Paris (60), post office (61), bank (62) | black, signal icon (now the DS's pattern, below) | black (`solo_post_walked.png`) | right (as alone) |
| Books building (53) | the shelves above (3D) | same | right |
| Lovers Point (49), beach (64), Fungi Forest (66) | the sky / the forest's canopy | same | right |
| Area 63 | the trees' dark trunks | same | right |
| The other world: the road (38), the tower (59) | the tower / black with signal icon | | right |

**The DS's session pattern in rooms (fixed 2026-10-28).** The DS shows its session pattern (light blue diamonds) on
the top screen of every room in a session; with the town on the top screen the rooms showed single player's black
instead, because they are always entered from a field with 3D on both screens. The pattern appeared only after a map
warp from one such room to another (`s2_cafe.png`, `s2_bank.png`). **Cause** (a research hook on the VRAM queue's add,
main 0x02006958, logging the callers; not kept): the pattern is the sub engine's BG1 (BG1CNT_B 0x220E: tiles at
0xC000, map at 0x1000, palette 1, three light blues), loaded **once per field load in a session** by the field's
session top-screen object (main 0x0203A5D4 -> 0x0203A764, from the field's load ov001 0x020C7B42 -> main 0x02011920):
file 6 of the archive *0x020A40D0 read into a buffer (main 0x02004978), opened (main 0x0200A2BC), its tiles, palette
and map queued for BG1 of the object's engine (main 0x0203AB0C, obj+12 = 1, the sub engine) and the buffer freed
(main 0x020035C0). Entering a room, that load runs while 3D on both screens still runs: the upload helpers then keep
an upload only on the frames that draw its screen (engine = frame parity, else dropped; the host lost the tiles and
map, the guest kept them), and the room's set-up then ends 3D on both screens (main 0x02002934 -> 0x02002710 from the
VBlank), which clears the sub engine's BG palette and hides BG1. (The queue's kind word is type | layer << 8 | engine
<< 16; the communications screen's own uploads, 0x023B2A14..., are the main engine's, not the pattern: replaying
uploads seen on the dual-screen frames mixed the two.) **Fix** (`mq_dual_session_top`, game/src/cfie_hooks.cpp, from
`mq_frame_end`): for a session field whose set-up was left with 3D on the bottom screen only (`mq_room_field`, set by
`mq_dual_in_session`), once that end of 3D on both screens is done (byte 0x020ACA54 0, no change pending at
0x020ACA55 bit 1), the field is up and 4 frames have passed, the game's own three steps run again through `ntr_call`
on a scratch stack (file 6, open, main 0x0203AB0C with a stand-in object whose engine is 1, free) and BG1 is shown in
the sub engine's layers (0x020AD6D9): the next VRAM update loads the pattern, the same picture as with the Pattern
(DS) setting (pixel counts identical). Once per field load; skipped when BG1 is already shown (the game's own pattern
after a room-to-room warp) and in area 16 (its set-up clears both engines' BGs in a session). Leaving, the next field
with 3D on both screens sets its own layers (the town's top screen back). Single player and the Pattern (DS) setting
are unchanged. `MQ_DUAL_LOG=1`: "the session pattern on the room's top screen". Verified by real input in a connected
pair (day1done.sav host, second_day1.sav guest, Connect now, then from town by each door with A): the café, the post
office and the bank on both consoles, each top screen the pattern (shots in `/home/claude/run/live/shots/pattern/`:
`bank_host_guest.png` both consoles in the bank, `cafe_post.png` the guest in the café and the host in the post
office), and walking out the town on top again (`left_to_town.png`). QUICK PASS.

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
    Map 38 is a road to a tower; the tower's door leads to map 59, its inside (seen 2026-10-27; first thought to have
    no door). Mystery 32's scenes: 102.2 on arriving in map 38, 102.3 talking to the old man (actor 212) in the
    tower, which ends with `warp_map 14`, back to the dorm, then 102.4 there; that scene is the only way back.
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
  student's position (0x020B8B7C, which is the bottom camera's point, not the student: "The student's real place"
  below): `warp_map 64 (5, 14)` puts the student at (5, 25.85), the dorm's (15, 1) at
  (14.95, 10.74), the hall's offset another; a partner warped to the first's *position* landed 12 units off, in the
  sea. The place a warp asks for stays at 0x020AD7F0 (x, y, z in 20.12, the map at +12; ov001 0x020D15B4 writes it)
  until the load takes it: caught each frame, it is what is sent for a numbered map (the town: its arrival points are
  picked by the position). The mysteries' joins (coop_myst.cpp) had the same fault; they now convert the position
  with each map's offset ("Mysteries together", "Where beside is").
- **Back**: when the first student leaves the destination (a door to town, or mystery 32's scene to the dorm), a
  partner still standing in it is asked "<name> is heading back. Go back with <name>?" and taken to where the first
  went: into the town at the arrival point nearest to the first (warp_local, 90033), or a numbered map (warp_map).
  From the other world, which has no way out, the partner goes back without a question ("You go back with <name>.").
  The other world is two maps (the road 38 and the tower 59): walking through the tower's door is not leaving it, for
  either student (before 2026-10-27 the partner was told "You go back with <name>." when the first walked into the
  tower). Either can also walk out by the destination's own door at any time; that ends the trip for them.
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

### The student's real place (fixed 2026-10-26)

The co-op code read the student's position at 0x020B8B7C, which is the bottom camera's point (field+0x168), not the
student: south of the student and up by an amount that depends on the map and the place (town 3.4 south and 8.7 up at
the school's front and the dorm's square, about 7 at the plaza; the dorm 8.22, the beach 11.85, Lovers Point 12.23),
and it keeps the old place for a while after a map change. The "frame offsets" coop_myst learned per map and the
warp places coop_gate sends were compensating for that. Now `mq::student_pos` (`game/src/mq_student.hpp`) reads the
field's local player object, `*0x020AD7EC + 0x383C`, x, y, z at +0x2C/+0x30/+0x34: the place the talk finder measures
from, in the scripts' frame (warp_map 64 (5, 14) lands at (5, 14)). It reads (0, 0, 0) while a map loads (some 100-150
frames, while coop_gate's "loaded" test already passes); callers skip those frames (`real` false): no jump, no
settling, no learning from them. Users: coop_myst (the scene's start message, distances, the landing checks and the
offset learning), coop_gate (the place sent for the town, the settle after a map change), coop_hall (the auditorium's
offset, now (0, 0), used to put the first round's classmate back in the row), and the test switch `MQ_POS_FILE`
(`MQ_POS_CAMERA=1` for the camera's point). The learning is kept as a check and for a partner on an older build (its
camera point and its camera offset still give the place); a landing within 6 units (|dx| + |dz|) of the place asked
for counts as the same frame, pushed aside (the connection's warp into the dorm lands a guest at (14.27, 3.72) for
(15, 1) with the host standing there).

Verified (pairs of day1done.sav on one PC, Connect now, `MQ_POS_TRAIL` logging student and camera; screenshots in
`/home/claude/run/live/shots/`, outside the repo):
- RAM: in the dorm student (14.27, 6.00, 2.52), camera (14.95, 11.15, 10.74); at the school's front student (44.00,
  3.00, 23.91), camera (44.00, 11.70, 27.36); at the dorm's square (76.55, 3.00, 42.75), camera (76.55, 11.70, 46.21).
- Offsets learned now: the dorm (0, 0) ("a script's (15.00, 1.00) is the position (15.00, 1.00)"), the beach (0, 0) on
  both consoles (were (0, 8.22) and (0, 11.85)).
- A mystery join in town (`--clock 2000-05-24T10:00`, mystery 51 stage 1 poked on the host, its set-up 12130 on both,
  12102 started on one, the other far away asked "Go and help?", Yes): "a step beside the other student", event
  90018; twice: the host at (77.54, 42.75) beside the guest at (76.55, 42.75), and at (45.00, 23.91) beside the
  guest at (44.00, 23.91) at the school's front, one unit apart, side by side on both consoles
  (`pos_myst_4_beside.png`). With the camera's point the step had gone to a place 3.4 units south of the other.
- The Gateway's "go with" (`--clock 2000-01-18T10:00`, the guest in town, the host's Gateway, Anywhere): the beach, host
  (5.0, 14.0), guest "to map 64 (6.0, 14.0)" and standing there (`pos_gw_beach.png`, both students side by side on both
  consoles). Back: the host walked off the beach into town (landing (65.75, 5.38)), the guest "Go back with" Yes: "to
  the town at arrival point 232", the nearest to the host, at (60.76, 6.23). (A build that did not skip the zeroed
  frames sent (0, 0) here, the place read while the town loaded, and took the guest to arrival point 206.)
- The tournament (`--clock 2000-01-23T15:30`, the twelve incantations, s225 = 3, 0x020AD7FE = 1; the host into the
  auditorium, the guest asked and Yes): "the auditorium's offset (0.00, 0.00)" on both, both met at the rules, "duel:
  the first round is against the other student", the guest first: "slot 1 wins", on the guest "the first round's
  classmate 68 back in the row" (`hall/posD_b_begin.png`, `hall/posD_c_after.png`).
- QUICK PASS.

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
- **The refusals are lifted** for World Tree, Candyman, Werewolf, Homunculus, Familiar, Golem, Charm, Phantasm (their "if
  t212 != 0 go to <the refusal ending in 999:3120>" made a test that never holds) and Connect doors, Sweet dreams,
  Make peace, Sit by me, Honor student, Mushroom (the "if t0 == 0" after query 0xB5, when t199 is one of them):
  the caster's console runs the event as alone. Seen for each, cast by the guest (and Golem and Sit by me by the
  host): "You succeeded in using the ... incantation!" (Make peace and Sit by me: "You have properly prepared for
  ..."), both consoles still connected and free to move afterwards.
- **Phantasm** (lifted since: "Phantasm in a session" below). Before that it stayed refused: its scene reloads the
  caster's map (query 0x14(1), warp_back) to meet the fortune-teller.
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
  the test only).
- **Phantasm in a session (built 2026-10-26)**. Its records 8 and 11 want field+0x3EB5 = 1 and area 4 or 5 (main
  0x02047650 -> ov001 0x020D6DC4: an area under 0x51 is compared as it is): maps 4 and 5 are **the dorm's
  washrooms** (the mirrors; `MQ_WARP_FILE` `4 5 4`). What the scene does (event 350 block 1): the cast's sparkle,
  query 0x12(1), query 0x14(1) (field+0x1522D: the warp keeps the screen white), warp_back (the map reloaded where the
  student stands), a nowait block that places the fortune-teller (actor 173, "Phantasm") and the student by t170
  (query 0x34: which washroom), the camera, "You succeeded in using the Phantasm incantation!", her talk (queries
  0x93/0x94 pick a classmate whose friends she tells about; they read the classmates, nothing written), s69 = 1 (met
  her: a later cast only says "You succeeded"), s479, and the first time (g1383 = 0) op 0x73 (an entry added to the
  town block's list at +0x6FF0, its count at +0x6F0C) and the town's g1383 = 1, g1410 = 1 (read by 260 and 223).
  Alone it plays to its end (verified, a solo copy in map 4 with the cast switch).
  **What went wrong in a session** (the guest and the host alike, map 4, `MQ_MAGIC_LOG`, gdb on the recompiled
  functions): warp_back (ov028 0x0211384C) sets 0x020FE29A = 1 and sleeps until the field is there and the byte is
  no longer 1; the reload's set-up (ov001 0x020CDF2C, from ov003 0x020FF6C4) makes it 2 and runs the event's next
  commands itself in its load mode (main 0x0201C4C8 with 1: the nowait block), while the event's own thread (main
  0x0201C498), let go by warp_back, skips the nowait block and waits in query 0x11 (field+0x3E7C, the field ready).
  Before each command a session event's commands are sent to the other consoles (main 0x0201C648: for the start
  kinds 0 and 3 of the context's +0x23BC and the commands of a table, main 0x0209CF48 / 0x020A3771) and the sender
  sleeps until each is sent; alone there is nothing to send and the load's run goes through at once. In a session
  the load's run slept on its first command, the event's own thread stepped the shared command pointer past the
  nowait block, and the load's run, resuming, read a command of size 0 and ended the event (main 0x0201C542; seen
  with a hardware watchpoint on the context's +8): no fortune-teller, the student behind the white screen of query
  0x14(1) (`/home/claude/run/live/shots/fish2/ph_5.png`; the game runs on). (Holding the event's thread with the
  context's +9 stalled the load instead: field+0x3EAE stayed 1.)
  **Built** (game/src/coop_magic.cpp, only in a session): Phantasm's refusal (t212) is lifted like the other summons
  (the caster's console plays it as alone); while its warp_back waits (0x020FE29A = 1) and until the field stands
  ready again, the event's start kind is made 2 (as our own events have it), so its commands are not sent and the
  load's run goes through as alone; then it is put back (it was 3). The first meeting's town part (op 0x73, g1383,
  g1410) is the town owner's: on a guest "if g1383 == 1 go to the end" always goes there, so a guest's cast neither
  uses up the host's first meeting nor leaves an entry in its copy of the host's town that would be lost at the end
  of the visit. Verified (pair of day1done.sav, `--clock 2000-07-18T14:00`, Connect now, the caster warped to the
  washroom, cast with `MQ_COOP_CAST_FILE`; shots in `/home/claude/run/live/shots/phantasm/`): the guest's cast:
  "You succeeded in using...", "Are you the one who called for me?", her talk about Sammy's friends, "I may not look
  it, but I'm..." (`p2g_1_teller.png`, `p2g_2_succeeded.png`), the guest free in the washroom afterwards
  (`p2g_3_after.png`), the host told "AAAAAA cast the Phantasm incantation!"; RAM: only the guest's own record
  changed (+0x51B, +0x52F = 34, +0x556), the host's town and record unchanged. The host's cast: the same scene
  (`p2h_1_teller.png`: the guest in the same washroom sees the host, not the fortune-teller, which is the caster's
  alone), the host's town g1383/g1410 (+0x427 bit 7, +0x42B bit 2) and the +0x6FF0 entry, which the town sync brought
  to the guest's copy unchanged; both connected and free to move after. QUICK PASS. Not verified: a cast through
  the magic menu's words by pad (the switch does what the menu does after them).
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
  entering the magic tournament!", "Go along with <name>?", lines 490-491 of bank 60; a Yes after the offer has gone:
  "<name>'s evaluation has already begun." (492) or "<name> has already gone in to the magic tournament.", "You can
  still talk to Mr. Graves to enter it." (494), not the mysteries' "worked it out"): Yes takes it beside the other
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
- **All by pad** (2026-10-26, pair of day1done.sav copies, Connect now, `--clock 2000-05-21T15:30`, `var 411=0`,
  `20ad7fe=1`, `var 296`-`299 = 1` poked on both, then both into town by `e 239`; screenshots
  `/home/claude/run/live/shots/tp7/graves/`, outside the repo): the host walked to Mr. Graves, A: "Nice to meet you,
  ...", "Oh, what's going on there?", the menu Chat / Extracurricular lesson / Quit (`g3_host_menu_open.png`), Down,
  A: 510.3, then 521.1 by its record ("Hello, students."); the guest was asked "Go along with AAAAAA?" (`g5`), Yes: a
  step beside; it talked to Mr. Graves itself (from his east side, facing west: north of him the gathering's people
  stood in the way), Extracurricular lesson: its own 510.3 and 521.1 (`g10_guest_gathering.png`); both through the
  gathering and the auditorium's rules by A ("after 240 frames at the rules", "duel: the first round is against the
  other student", the host's draw 9, s298), Mr. Graves between the two students with their wands and the words open on
  both (`g14_words_open.png`); the words by the test switch on the guest: "duel verdict: slot 1 wins (... guest 939
  frames)", the host's tournament ended ("Well done, students!"), the guest on to round two against a classmate
  (`g16_after_duel.png`). No request was refused on the way (`MQ_MYST_ARB_LOG=1`).
- The far start that froze before (the host's gathering started by its record after a warp to the dorm's arrival
  point, z 47): with `mq_town_rows` "town rows: the camera moved 39 rows at once (row 44 -> 83): every row made
  again", the scene ran and the session stayed (`far_1_host_guarded.png`, the backdrop wrong as said above); the
  guest by Mr. Graves entered and went along, both met at the rules, the duel ran with the host first ("slot 0
  wins"), the host on to round two, the guest's tournament over (`far_2_duel_words.png`, `far_3_after_duel.png`).
  Alone the same start had ended the game ("fatal: no recompiled function at 00000000 (lr 020136ff)"); with the fix
  it plays (`far_solo_backdrop.png`). QUICK PASS.

**Open:**
- (Resolved, not a bug: the tests' set-up.) **Talking to Mr. Graves.** The tests found him and his blackboard not
  drawn and A by him starting nothing: the student stood by `MQ_POS_FILE` (then the bottom camera's point in town, about 7
  units south of the student), so it was about 8 units north of him, with him below the bottom screen. Alone, by
  pad, on May 21 (s411 = 0, 0x020AD7FE = 1, four tournament incantations), standing 1 unit north of him facing him:
  his talk ("Nice to meet you, ...", bank 32 line 1614 "Oh, what's going on there?") and his menu Chat /
  Extracurricular lesson / Quit; the lesson ran 510.3, the gathering in town (521.1, "Hello, students.") and the
  auditorium's rules (521.2). How to reach it in a test: docs/TESTING.md, "Talking to a festival's teacher or Mr.
  Graves for real".
- (Resolved 2026-10-27.) The duel's words by the stylus: verified by touch (docs/COOP_PLAYTEST.md, 12). A winner's
  words panel stays on the bottom screen after the round, with the scene and Mr. Graves' lines on the top: the game's
  own (the winner's branch, 521.2 #307, has no 0xB2; the next round opens the words again with 0xB2(1)), not a fault.
- The partner's own evaluation result is seen only on its own console (each student's text runs on its own); the
  other student stands beside.
- (Fixed 2026-10-27.) **A rank-up in a session hung both students at the staff roll's poll** (590.1 #634, query 0xA1):
  0xA1's own answer reads the roll's object at *0x02110060, which lies in overlay 23 in a session, so a non-zero word
  there read as "still playing". In a session 0xA1 now answers "over" (docs/COOP_PLAYTEST.md, 12).
- More than two students: the rules follow one other student.

### Telegrams in a session (fixed 2026-10-27)

What the game does (static: main 0x0204C4CC-0x0204D0EC, ov009 0x0210B04C, ov001 0x020F1470, event 225; seen in RAM):
- **Writing** (the post office, Thom, "Write a telegram", 500 Ritch, 216.2/5; ov009): the recipients are the
  **students who live in the town**, by their slot in it (the student's record +2: every save's first student is slot
  0), from a mask (town block owner 0x020AD814 +0xC, copied to the session's +0x6923 at connect: the host town's).
  A guest is offered "Other player" and the host's name; the **host is offered no one** ("Other player", then a list
  with only "Exit"): it is the town's only student, and the visitors are not in the mask. As on the DS.
- **Sending**: the entry (0xE4 bytes: text at +0, the sender's name at +0xC4, +0xDC/+0xDD a classmate's line,
  +0xDE the sender (0xFA another player, a classmate's number, or 0xFB + a slot), +0xDF the addressee 0xFB + slot,
  +0xE0..+0xE3 not traced) goes into the town's **waiting list** (town block +0x7AA8, ten entries, +0xDE = 0xFF free;
  the oldest pushed out when full) on the sender's console, and the session's packet 0x42 (arbitrated by the host,
  ov001 0x020F1470 kind 1) puts it in the host's: both consoles hold it (the town sync keeps it so).
- **Delivery**: the **mail cat** (actor 138). Entering the dorm (area 14), 225.2 asks query 0x85 (ov028 0x02118BB0 ->
  main 0x0204C830: an entry for 0xFB + the student's own slot?) and places the cat by the student's room door ((2, 7)
  for slot 0, s1 = 22..25). Talking to it, 225.3 asks query 0x8B (main 0x0204C884): each such entry moves into the
  student's own telegram box (record +0xEF4, ten entries; the Start menu's TELEGRAMS tab) and leaves the waiting list;
  the cat reads them ("Meow!", the text, "From: <name>", "Ah! But there's more...!", ...). Nothing is delivered
  anywhere else (not in the room, not by Thom, not by the day's start).

**Two bugs, fixed** (both without regeneration):
1. **The cat had no model in a session** on the host: 225.2 placed it at (2, 7) but a building's placement gives no one
   a model in a session, so it was never seen nor talked to and the telegram stayed waiting until the host played at
   home alone (seen: after the Disconnect, at home alone in the same run, the cat came with a model and delivered
   it). Now `scene_people_models` (`game/src/coop_class.cpp`) gives the cat a free model in the dorm's halls (only
   the cat there, only once the other players' slots are known, as the tower's people): "coop class: person 138 in
   area 14 gets model 2".
2. **A guest would take the host's telegrams**: its own student has slot 0 too (of its own town), so in the host's
   dorm 225.2 placed the cat for it (seen in a guest's RAM: actor 138 at (2, 7)) and 225.3 would have moved the host's
   telegrams into the guest's box. A visitor has no mail in the host town: on a guest the query hook
   (`mq_hall_query`, `game/src/coop_hall.cpp`) answers 0x85 "none" and 0x8B "nothing handed over" ("coop hall: mail
   query 85 on a guest: no telegrams here").

**Verified by pad and touch** (2026-10-27, two different students: day1done.sav's AAAAAA and the second test save's
NORA, docs/TESTING.md; Connect now, `--clock 2000-05-03T10:30`; shots outside the repo in
`/home/claude/run/live/shots/tg/`): NORA (guest) wrote "HELLO AAAAAA" (To: AAAAAA / From: NORA, `g2h_text.png`);
the guest then walked into the host's dorm: no cat (the log line above); the host walked in: the cat with model 2 by
its door (`h_dorm2_cat.png`), A: the two classmates' waiting telegrams, then "HELLO AAAAAA" / "From: NORA", the
waiting list empty on both consoles, three telegrams in the host's TELEGRAMS tab (`h_tab5.png`), in the session. The
other way (NORA hosting, AAAAAA guest): "HI NORA FROM AAAAAA" (`a2n_text.png`) delivered to NORA by the cat in the
session; the guest in NORA's dorm saw no cat. Before the fix (the same steps): the cat stood without a model on the
host and was placed for the guest too; after the host's Disconnect (both saved), the host at home alone got its
telegram from the cat (`t29b_cat.png`). QUICK PASS.

### The actions of the partner's copy (audited 2026-10-27)

Each console shows the other student by a **copy**: a remote player object (`*(field+0x3E4C)`, 3 x 0x638) moved by
that student's console's 16-byte packets. The copy is not only drawn: the game *plays* its actions with its own code.
A packet of type 0x13 sets the copy's action (+0xA2) through main 0x0201E4DC (main 0x02031404's 0x13 case, packet
byte 3); other packet types set the others directly. Once a frame, while the copy stands in this console's map
(+0x62D = field+0x3EB4), its **act function** (main 0x020325E0, its class's vtable +0x64) runs the state function
of that action: for most actions the **same function the local student runs** (the local dispatch, main 0x0202211C).
Those functions test the student's +0xAA (1 = the local student) before what reaches beyond the student, but not
everywhere: the net's sweep through the air ran the catch test for any student, so the partner's copy caught (and
lost) a butterfly on the other console ("Insects together"). This section is that bug's class, for every action.

How the table was made: [S] the act's dispatch (main 0x020325E0, a switch on +0xA2 into 17 cases), the tool
sub-dispatch (main 0x02029C60, on the held tool 0x22-0x28), the 0x13 sender (main 0x0202BBFC: which of the local
student's actions are sent at all), and for each function the copy runs: every packet send (main 0x0202BD54 /
0x0202BDA8 / 0x0202BE08 / 0x0202BFAC / 0x0202C1E0 -> 0x0202C4CC), store write (main 0x0201E070), catch test, bag
write and direct field write reachable from it, each with its +0xAA test or not. [V] `MQ_ACT_AUDIT` (below): main
RAM compared before and after every act of the copy, in a connected pair over the internet mode at 200 ms
(`NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`, host day1done.sav AAAAAA, guest second_day1.sav NORA), each action done by
real input on one console while the other watched; and the guard's own log (`MQ_COOP_ACTS_LOG`) in the same runs.

| Action (+0xA2) | What it is | Sent as | The copy runs | World-changing code in it | Status |
|---|---|---|---|---|---|
| 0, 1, 14 | standing, walking (14: the copy's walk) | 0x0F/0x13 | nothing (0x13's own anim) | none | [V] nothing in the audit |
| 2 | talking (A on a person) | 0x1E | main 0x02032DDC (copy only: anim, face) | none | [V] player talk: nothing |
| 4 | a hand tool: net 662, the 663 tool, ... (R cycles 660-663) | 0x24 | main 0x02029C60 -> net 0x02029F2C, 0x25 0x0202A85C, 0x22 0x02029E38, 0x26/0x28 inline | the net's sweep: catch test for any student (main 0x0202A0AE) | **fixed** (bf19704, `remote_catch`); the rest [S] behind +0xAA; [V] 663 and the net's sweeps: nothing else |
| 4 (shovel 660) | digging | a ground object packet (0x33-0x41) | (no action on the copy) | the hole: the host arbitrates | [V] the copy shows no action 4 for it |
| 7 | through a door, arrivals | 0x13 (only back to 0) | local 7's own (step 4) | none | [V] the guest into the bookshop: nothing |
| 8 | taking something from standing (a mushroom, a ground item) | 0x13 (also for 16) | main 0x02023540 (the local student's) | step 0: **clears the field's catch view** (field+0x135FE, main 0x0202358A, no +0xAA test); with the copy's bag full in areas 22-25: **the field's message** (ov001 0x020CEF7C) and *0x020B8AC0+0x23BC = 2; step 9's 0x41 send is behind step 6's +0xAA test (unreachable for a copy) | **fixed** (the guard); [V] below |
| 11 | the rod (cast, bite, reel; 661) | not 0x13 (another type) | the case only ends itself (step != 0 -> 0) | none | [V] fishing by the guest: the copy in 11, nothing in the audit; the fish: coop_fish.cpp |
| 12, 13, 15, 22, 26, 33, 41 | 12/13 posture, 15 into the bag, 22 holding an item up (casting), 26 putting an item down, 33, 41 | 0x13 / 0x1D / 0x16 / 0x28 | the local student's functions (0x02024A08, 0x020227B8, 0x02023BF8, 0x020251FC, 0x020244E8, 0x02024D5C, 0x02026110) | every send, bag and store write behind +0xAA [S] | [S]; 26/22 not reached by pad in the time |
| 16 | a catch held up when the bag is full ("Select an item to remove") | as 8, only from standing (not after the net's 4) | (as 8) | as 8 | as 8 |
| 17, 19, 31, 46, 5 | copy-only cases (17 main 0x02032E84, 19 ov001 0x020CD274/0x020CD694 seats, 31/46 back to 0, 5 a timer) | 0x26, 0x17, ... | copy-only code | ov001 seat code tests +0xAA (0x020CD310) | [S] |
| 23, 27, 28 | chanting, Friendship / Declare love | 0x13 | nothing (not in the act's switch) | none | [V] the wizard form and back by the guest and the host |
| 30 | turning into a wizard (the D-pad circle) | 0x23 | main 0x020256C8 (the local 0x02025508's copy part) | flag 510 written into the *copy's own* record (+0x4FC), not ours | [V] nothing in the audit |
| any (anim 0) | a student's standing animation (main 0x0202014C) | - | for the copy too | writes its +0xA8 into the **town's global store byte 74** (town+0x322), unless flag bytes 33/34/214/215 say otherwise: on a guest that is sent to the host as the guest's own | **guarded**; [V] not seen written (a write watch on it stayed quiet in all runs) |

**The bug found and fixed** (the guard): a player's bag is full and it catches an insect; the game holds the catch
up (action 16, the field's catch view +0x135FE = 1) and asks "Select an item to remove" (step 25), and the choice
reads the catch view at its end (main 0x020243D8: 1 the insect, 2 a fish, else the item is put on the ground). If
the partner meanwhile takes something from standing (a mushroom, A), the partner's copy runs action 8 on this
console and its step 0 clears the catch view here. Seen with the guard off (`MQ_COOP_ACTS_OFF=1` on the host; the
host's bag poked full, the insect caught by pad, the guest's mushroom by A 47 units away): the catch view 1 -> 0 on
the host each time (3 runs); once (lostD) the host's choice then ended by itself, without any input: the insect in
bag slot 0, that slot's mushroom put on the ground beside the host (grid (18, 66) = 266), the host's action 0.
With the guard (lostE): "coop acts: the partner's copy (action 8 step 2) wrote field (catch view)+0x135FE (1 -> 0)
here: undone", the host still asking "Select an item to remove from your item window." with the catch view 1
(`lostE_end.png`). docs/COOP_PLAYTEST.md has the runs.

**The guard** (game/src/coop_acts.cpp, `mq_remote_act`, instead of main 0x020325E0): in a session the copy's act runs
as the game has it, then every byte it changed of the town block (except its time object +0x2C8, each console's
own), the local student's record (0x020AD824, 0x1858 bytes), the field's catch view (+0x135FE) and message request
(+0x151BC..+0x151BF) is put back; on a guest the variable store's writes made inside it are not taken for the
guest's own (coop_sync.cpp skips them). The copy's animation and its own object are untouched. Only the console of
the student who acts decides those, as for the insects. A thread switch inside the act (seen once, while a held
item's model loaded) leaves the wrapper without its end; the frame end clears its state. Not covered: the packet
handlers themselves (main 0x02031404, the game's own exchange for ground objects, talk and the like, which the
host arbitrates).

Open: once, with the guard off, the host crashed in the same situation (its bag full, choosing, the guest taking a
mushroom; `lostC`): `fatal: no recompiled function at 00000000 (lr 02031c7f)` in the copy's 0x1D packet case (main
0x02031B50: the take's display, a call through the copy's model +0x1E8). Not seen again in three more runs of the
situation (two with the guard off, one with it); whether the guard prevents it is not known.

Research switches: `MQ_ACT_AUDIT=<file>` (the before/after compare of every copy act, 4 MB, slow; also the local
student's action and step changes), `MQ_COOP_ACTS_LOG` (each byte the guard puts back), `MQ_COOP_ACTS_OFF` (no
guard). The town's ground grid for tests: `*(field+0x968)` = town+0x438, u16 per cell, `x` 4-83, `z` 4-99, index
`(z-4)*80 + (x-4)` (ov001 0x020D4F3C); mushrooms are items 256-279.

### What a session refuses, and where (catalog)

Four mechanisms decide it:

- **A, menu text:** a tag `$016'A'B` in a menu option jumps to message B instead of A while in a session (main menu
  parser 0x02038938, the switch at 0x02038A9A). Used by the CD shop (buy/pick up, 5150), the general store's part-time
  work (5353), the flower shop's berry seedlings (5780) and the post office's Download (8300).
- **B, flag byte 212** (script variable 10212): `session[0x6912] + 1` while connected (set ov001 0x020F2300, cleared at
  the session's end, ov001 0x020EE0E6 / 0x020EE142 / 0x020EE1A6). Scripts test it with 0x38: the school bell (8614),
  registering in the insect and fish books (8634, 8649), the locker's explanation (8673), the storage chest (999:1021),
  the café (32:6230), one room door (999:5306), and the summon and transform incantations (3120: World Tree, Phantasm,
  Candyman, Werewolf, Homunculus, Familiar, Golem, Charm; all lifted, "Incantations in a session").
- **C, query 0x22:** in a session (variable 10020) and the console's slot (variable 10000), ov028 0x021152D0: the
  communications room (no second session), shopkeepers' walks.
- **D, query 0xB5** (ov028 0x0211A16C): the incantation's result id is flag byte 199. Connect doors, Sweet dreams, Make
  peace, Sit by me, Honor student and Mushroom are always refused (3114-3119; lifted in co-op); Rainbow, Flower rain and Rainmaking ask
  the other console (packet type 0x45) and Friendship and Declare love also wait for the target's state: these are the
  game's own two-player incantations.
- The bank (deposit, withdraw, safe deposit) uses another menu for a guest: George's greeting is 32:5806 instead of
  5804/5805, and its `$018'2` (go on two lines) leads to menu 5808 instead of 5807, whose three services go (`$019`)
  to 5860 "you can't use that service while visiting another town" (the host keeps 5807). Which greeting is a talk table's choice (not
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

**A classmate both students talk to** (`mq_coop_npc_talk_ok`, ov001 0x020F08D4 `instead`, game/src/coop_myst.cpp):
an NPC talk asks the host first (request 53, +14 the NPC), and the host refused it while another console talked with
that NPC (0x020F0EDC, kind 1): the second student's A at the classmate did nothing. Each console runs its own copy of
the classmate, so now a talk taken by another console is allowed too (not recorded); a free one is the game's
(docs/COOP_PLAYTEST.md, 11). `MQ_COOP_NPC_TALK_GAME=1`: the game's refusal.

**Another console's script writes** (`mq_coop_remote_cmd`, main 0x0201C770 `instead`, game/src/coop_myst.cpp): in a
session the interpreter sends some commands to the host (request 0x39 with the command: copy_var 0x5B, set_student
0x3B), and every console runs the answer for the console that sent it (ov023 -> main 0x0201C770, the player = that
slot). On the DS no scene runs on two consoles at once; here each console runs its own, so a copy_var into the
script's own bytes (variables 10000-19999, the running scene's flag bytes) from the other console's scene wrote this
console's scene: the guest's homeroom swap made the host's farewell say goodbye with another classmate. Now another
slot's writes of variables 10000-19999 (copy_var into them, set_local 0x3D, add_local 0x3E) are not run on this
console; writes of a student's record or the town's still are. Logged "slot n's command 5b writing tN not run here".
`MQ_COOP_REMOTE_LOCALS=1`: the game's way.

**Shopkeepers across the counter** (`game/src/coop_talk.cpp`, `talk_to_shopkeeper`, in the talk hook): in a session
nobody could talk to a shopkeeper standing behind their counter. Out of one, the shopkeeper walks about (the full
brain) and comes within the talk finder's reach (ov001 0x020C875C: 2.25 units for 126/127/128/129) of a student at the
counter; in a session every NPC gets the online brain, whose think does nothing, so the shopkeeper stays where the
shop placed them, 2.5 units from the counter's front (the finder's own numbers, Harrows: Piquard at z 24, the student
stopped at z 26.5), and pressing A did nothing. Now, in a session only, when the finder finds no one and the other
student is not in front, a shopkeeper (NPC 126-133) that passes the finder's own tests (a trigger, shown, not left
out, level, within 60 degrees, its own "can talk") within its reach plus 0.75 units is talked to with the talk call's
own steps (main 0x02028132-0x0202819A). Seen at Harrows (both floors), Daisy's and The Emporium (and the bank's George).
Still refused: Download, a second session. (Phantasm and the other summons
and transforms are lifted: "Incantations in a session".) (The trash can, the school bell, the bank's deposits and a guest's own safe deposit are lifted: below.)

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
**The safe deposit: the host's as alone; a guest's own, served from its own save** (`game/src/coop_trash.cpp`,
second part; built 2026-10-27). Its items are kept as counts in the *town* block, 4 bits per item slot from +0x8
(main 0x02011198 reads, 0x020111CC writes; the slot is main 0x0203A024's for the item, 0-0x549; 0x2A5 bytes to +0x2AD,
whose last byte also holds a flag bit, main 0x02010F24's bit 0x1527, so of +0x2AC only the low half is taken), not in
the student's record: it is the town owner's safe deposit. Only a guest gets George's menu 5808; the host's George
shows 5807, so the host's Use safe deposit was never refused (checked 2026-10-26: its Beige Wardrobe went from its bag
to its town's +0x196 and the town sync brought the count to the guest's copy: `shots/bank/h_screen.png`,
`h_dragged2.png`). A guest's own safe deposit is in its own town, which is not in RAM during a visit, so the game
refused it ("I'm sorry, but you can't use that service while visiting another town.", `shots/bank/g_refused.png`).
What is built, a guest in a session only:
- **Its own counts from its own save.** When the guest stands in the host's town (coop_sync's slot known) it takes its
  own counts from its save memory (the save is 64 KB: a header, the town block at +0x20, its checksum at +0 checked,
  the student's record at +0x9960; a save programs the town +0x20-0x995C, the record +0x9960-0xB144, then the header
  +0x10-0x20, a page at a time over some frames). Menu 5808's *Use safe deposit* then keeps its own target (5987,
  `mq_bank_target`): "So you would like to use the safe deposit, correct?", event 214.4, query 0x55.
- **Its own counts in the screen.** When 214.4's query 0x55 runs (`mq_hall_query` calls `mq_safedep_query`) the
  host's counts in RAM are put aside and the guest's own put in their place; the SAFE DEPOSIT screen (overlay 18: its
  init reads every item's count once, ov018 0x0210C564) shows and changes them as alone. Two frames after event 214
  has ended the counts in RAM are the guest's new ones (kept, "our own safe deposit changed: slot 796: 0 -> 1") and the
  host's are put back. The town sync never sends these bytes (a guest's writes outside the variable store stay on the
  guest), and the host's bytes for them that arrive meanwhile are put aside with the host's (`mq_safedep_hold`, called
  from coop_sync's town bytes): the host's safe deposit is never written by a guest.
- **Saved only with the student** (an item in the bag and in the safe deposit would be a copy, in neither a loss). The
  game saves a guest's record at the visit's end ("Disconnected.", "Saving.": it puts the guest's own town back in RAM
  and saves the town and the record), never during a visit. The runtime now calls a hook after each write of the save
  memory (`ntr::nitro::set_backup_write_hook`: the synchronous path and each page the emulated card programs, before
  the frontend is handed the save): when the game's write completes the record (the page that reaches +0xB144) the
  guest's counts are written into the save memory's town block and its checksum fixed, so one save file write holds
  both. Then, once the field stands loaded at home (outside a session), the counts go into the town in RAM too (the
  home load takes the town from RAM, not from the save: seen "home: our own safe deposit in our town: slot 796: 0 ->
  1"), so a later save from home keeps them. A guest's state is kept from the session's end until then (the save comes
  after the session object is gone).
- **A visit that ends without the guest's save** drops what was not saved: the game gives the console its student as
  last saved; a console that is closed or drops before the visit's end has neither the bag's change nor the counts in
  its save (both as at the connect save).
`MQ_COOP_SAFEDEP_GAME=1`: refused as the game has it; `MQ_SAFEDEP_LOG=1` (each query in event 214, the host's bytes put
aside), `NTR_BACKUP_LOG=1` (each save memory write); `MQ_MENU_LOG=1` names the bank's options.

Verified 2026-10-27 by pad and touch (pairs of day1done.sav, AAAAAA, host, and saves2/second_day1.sav, NORA, guest,
same-PC link, Connect now, `--clock 2000-07-18T14:00`; harness `/home/claude/run/live/sdpair.sh`, `sdlib.py`,
`sdrun2.py`, `sdrun5.py`, `sdsolo.sh`; screenshots `/home/claude/run/live/shots/sd/`):
- The guest at George, *Use safe deposit*, its own SAFE DEPOSIT screen (`g05_screen.png`), its Beige Wardrobe dragged
  from the bag into it (`g06_dragged.png`), OK, "Thank you.": the guest's bag empty, its own count for slot 796 1, the
  RAM town (the host's) unchanged, the host's bag and safe deposit unchanged. Its Disconnect: "Saving.", "Save
  complete." (the counts written with the record's last page), at home the counts in the town (`r1_10_home.png`); the
  save: bag empty, slot 796 = 1; loaded alone, the wardrobe in its own safe deposit (`s1_08_k1.png`).
- Withdraw in a visit (the guest's save with the wardrobe in its safe deposit): furniture, its fifth kind (list 14, the
  wardrobe 23rd), the list scrolled with the arrow, the wardrobe dragged to the bag (`r7_11_found.png`,
  `r7_12_withdrawn.png`), OK: bag [wardrobe], own count 0, logged "slot 796: 1 -> 0"; deposited again
  (`r7_15_deposited.png`), Disconnect, the save: bag empty, slot 796 = 1; loaded alone, found there (`s2_03_found.png`).
- The host's Disconnect after the guest's deposit ("The person you were playing with has disconnected", the guest's
  save, home): the guest's save bag empty and slot 796 = 1, the host's save and RAM unchanged (bag [wardrobe], no
  counts) (`r6_*`).
- A drop in the middle: the host's game killed (SIGKILL) after the guest's deposit: the guest (same-PC link) stopped
  waiting for the host's frames; its save still bag [wardrobe] and no counts, as at the connect save, and loaded alone
  so (`r5_*`): no loss, no copy.
- The deposit list: an item shows in its kind's list at its place among that kind's items (blank rows are the kind's
  items you have none of), so a list is scrolled to find it; the screen's own list of the furniture's kinds is
  lists 10-14 (the fifth, list 14, holds the wardrobes). QUICK PASS.

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
