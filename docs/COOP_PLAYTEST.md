# Co-op playtest: a week together over the internet mode

What two players do in a week of co-op, played as a scenario on one machine over the internet mode
(docs/COOP_INTERNET.md: four processes, each player's copy with its hidden stand-in), with a realistic link, where
features meet; what worked, what broke and was fixed, what is still open. Played 2026-10-26.

## Set-up

- Two players' copies started from copies of `day1done.sav` (the first day done), `--coop-internet` with
  `--lan-host` / `--lan-join 127.0.0.1`, `NTR_LAN_DELAY=50:10 NTR_LAN_LOSS=2` on both (50 ms each way with 0-10 ms
  jitter, 2 % of every datagram dropped: a round trip of 118-138 ms measured, the window's title "other PC 125 ms"),
  `NTR_DRAW_EVERY=10`, software renderer, unpaced. Harness (outside the repo): `/home/claude/run/live/wkpair.sh`
  (the pair with poke / event / goto / cast / connect files for each copy) and `wk.py` (the steps: connect, walk,
  talk, chat, gift, disconnect, RAM reads; every step written to a log with pair screenshots,
  `/home/claude/run/live/shots/wk/`).
- Real input where it was feasible: the pad driving each student (`drive.py`'s `goto` / `walk`), A on doors, on
  Principal Sol, on Mr. Graves, on Daisy, on the other student, Select for Disconnect, the Gateway touched with A.
  Test switches where the game needs the stylus or a story state the save does not have: `var 411=0` (the
  principal's talk about the Gateway, needed by the town's set-ups, mysteries and the Gateway's destinations) poked
  as Connect now starts, `MQ_COOP_CAST_FILE` for Friendship (the incantation's words are drawn on the touch screen),
  `MQ_GATE_ANYWHERE` (the day's Anywhere flag), arrival-point warps (`e 240` the school square, `e 239`, `e 234`) to
  skip walks already played.
- The week: sessions on different days and times, each from the saves the last one left, as players who play in the
  morning, come back in the afternoon and at night: S1 Tuesday 2 May 2000 08:40, S2 Wednesday 3 May 15:30, S3
  Thursday 4 May 03:50 into the new day. (Real time: an in-game day takes a day; unpaced copies run at 30-60
  frames a second, so a session covers minutes to an hour of game time.) S3 was played twice (S3a, S3b) and a last
  session S4 on Friday 5 May 09:00; S1 was played six times (S1a-S1f) as fixes went in.

## What was played (S1-S3) and what was seen

| step | expected | seen | result |
|---|---|---|---|
| Connect now (Connect automatically As host / As guest) | both in the host's dorm by the Gateway | 107-227 s from start to connected; "both in the session" 13-23 s later; RTT ~125 ms | works |
| Walk out of the dorm together | each sees the other outside | both outside, each console shows both students at the same places (RAM: the two consoles' copies of each student at the same place) | works |
| Walk to the school together, in through the door (A), upstairs | both in the hall | both in the hall by Principal Sol | works |
| Talk to Principal Sol (A) and pick a lesson | "Ah!", "So, which class would you..." | **nothing happened on A** (either player) | **fixed** (below: the players' talk broke every NPC talk in a session) |
| Lessons together: one player picks first, the other later | the first waits ("<name> is on the way"), both start together | the first player's **homeroom** (the farewell, 551) started alone at once; the second's minutes later | **fixed** (homeroom held like a lesson; a student in the school on the way to Sol is waited for) |
| Homeroom's farewell | the leaving classmate comes in and says goodbye | **hung** on the host: "So today's my last day..." for ever; the leaving classmate (68) had no model | **fixed** (the classroom's model loop) |
| Homeroom then the lesson, both | to "That's all for homeroom!" and "Well, that should just about...", each at the desk beside the other | both, on both consoles (`s1f_03_class_08.png`, `s1f_04_after.png`) | works |
| Chat (A on the other student, Chat) | greeting, menu, chat; the other told; pair records equal | both consoles "You chat with...", "<name> came over to talk", `.coop` lines equal (S1, again at night in S3) | works |
| Chat in the school hall with Principal Sol also in front | the NPC first (the game's talk) | nothing (the same NPC-talk bug) | fixed with it |
| A gift (Give an item, the bag's first slot) | the item leaves the giver's bag and reaches the other's | the Beige Wardrobe from the host's bag to the guest's (RAM: host bag empty, guest two); "You gave the...", "You put it in your bag." | works |
| Friendship cast together | "You succeeded...", both closer | "You succeeded in using... Friendship", pair friendship +2 on both | works; **the test switch** then left the other student stuck in the casting action (ignoring its movement on the caster's console): fixed |
| Gateway: Anywhere (the host touches it, A) | our menu (room / Anywhere / Exit), the partner asked, both there | our menu; the guest asked "Go with <name>?", Yes; both in the school's hall (map 67's session fallback), side by side on both | works |
| Back from the destination | the partner asked "Go back with <name>?" | asked, Yes; both in town at the school's arrival point | works |
| Mr. Graves in the plaza (A) | his talk | "Nice to meet you... Extracurricular classes are lots..." (after the fix); the tournament itself needs four tournament incantations and the 21st: not reachable from this save in this week | talk works; tournament not reached |
| Shop as the guest (Daisy's, through the door, A across the counter) | Daisy's greeting and menu | "Hello there. I'm Daisy...", "What do you need today?" (talk across the counter, `coop talk: to shopkeeper 127`) | works (the purchase itself was verified before, docs/COOP.md "Lifted in co-op") |
| The guest's Disconnect (Select, Yes) | the guest saves and goes home; the host "<name> is going home." | S1: the guest went home but the host never heard of it: its stand-in for the guest was dropped at the radio ("deauth") and the host kept a dead session; S2: the guest's Disconnect itself failed ("Disconnecting.", "Unable to disconnect.", "Unable to receive data.") | **fixed** (S3b: the guest's Disconnect ends as alone; afterwards, the host free: "<name> is going home." on the host, which plays on: below, 6) |
| Night: walk out, chat at 3:53 | as by day | both, the town's night sky on both consoles; chat as by day (`s3_01_night_out.png`) | works |
| The new day at 4:00 (spring), both free | the host's Disconnect, both home, both connected again into the new day | S3b: the host's "Disconnect?"... both saved and went home, the towns turned the day, both connected again at 4:01 (Connect now, ~2 min); "both in the session" | works |
| The new day with the guest left in an open text box (S3a: the last "You feel a little closer to..." of a chat not closed) | the same | the guest's stand-in host "Disconnecting.", "Unable to disconnect."; the guest stayed in the old session with it while the host, home and connected again, hosted a new one: each "playing together" with a stand-in | **fixed** (below, 7) |
| The next day's lesson at 4:0x after the reconnect | Sol, the lesson together | Sol answered at night too; homeroom 555 (the new student) hung on the host: one classmate got two models | **fixed** (no classmate's model made twice) |
| S4 (Friday 09:00): school, Sol, the lesson together | both | lesson 4102 (host) and 4103 (guest) side by side to "Well, that should just about..." | works |
| The guest's Disconnect, S3b (guest free, host in a hung scene) | the guest saves and goes home | "Disconnecting.", "Disconnected.", "Saving.", "Save complete." (after the fix); the host's stand-in for it then got "Unable to disconnect." (the host was stuck in that scene) | guest side works; host side verified afterwards (6) |

## Failures found and fixed

1. **No NPC within reach could be talked to in a session** (`game/src/coop_talk.cpp`). The players' talk
   (`mq_coop_talk`, the A press's talk try) first asked the game's own talk-target finder whether an NPC would
   answer, and only then let the game run its own finder. The finder's "can talk" test (an NPC's vtable +0x13C, main
   0x0202FF44 -> +76, main 0x0202E588 -> ov001 0x020E6580 -> 0x020E6544) gives the NPC the talk's priority at the
   town's director (`director + i * 32 + 0x17AF` = 8, `+0x17B0` = 1) when its priority is lower; a second test in
   the same frame then says no. So the game's own finder answered "no one" right after ours: Principal Sol (so no
   lesson could be picked by his menu: all earlier lesson tests had poked g1017), Mr. Graves, a classmate, anyone in
   reach (shopkeepers across a counter went through our own path and worked). Seen with gdb: our call returned
   Sol's entry, the game's call right after returned 0 (its "can talk" 1 then 0). Now our looks (the finder and the
   shopkeepers' search) put the director's priority bytes back; the chosen shopkeeper's "can talk" is asked once more
   as the game's finder would. Verified: Sol "Ah!" and his lesson menu for both players, Mr. Graves' talk, Daisy's.
2. **Homeroom started alone** (`game/src/coop_class.cpp`, `game/src/cfie_hooks.cpp`). Homeroom (551 the farewell,
   555 the new student; start records testing g1061) comes before the day's lesson and was not held like a lesson:
   the first player to walk in heard it alone and the second minutes later. Now held the same way (its record reads
   g1061 as 0 while it waits; the lessons' records after it wait with it). And a lesson or homeroom also waits for an
   other student who stands in the school (hall or classroom) with no lesson done today (a new state bit in the
   lessons' message 42), not only for one who has already picked: in play the second player is usually still walking
   up to Principal Sol. Verified: "homeroom 55101 held until slot 1's starts too", "begins (the other student is
   here)", both together.
3. **Homeroom hung waiting for the leaving classmate** (`game/src/coop_class.cpp`). The classroom's model loop
   (ov003 0x020FF0D4 -> ov001 0x020C5D20) takes the room's classmates in the order of the NPC list, not the people
   the placement kept (field+0x3EA4): with two free models, two classmates earlier in the list took them and the
   scene's leaving classmate (later in the list) had none, so the farewell waited for him for ever. Now a classmate
   the placement did not keep gives its turn to a kept one still without a model; and in a homeroom t131 and t133
   stay (its query 0x0C brings the people in before the scene swaps t131 and t133, so which one the story is about is
   not known yet when the placement runs). And a classmate that already got its model (a turn given to it earlier)
   gives the next turn on too (S3b: homeroom 555 made one classmate twice and the next had none). Verified: "classmate
   52 (not in this scene) gives its model to classmate 68", the farewell and the lesson to their ends on both consoles
   (S1f); S4's lessons with "classmate 68 (not in this scene) gives its turn to classmate 79".
4. **A guest's Disconnect over the internet mode** (`game/src/coop_net.cpp`). A player's copy shows the far player's
   DataSharing bytes in its stand-in's place. Those include the session's own steps (types 0x00-0x0D) when a console
   leaves: put in on the host, the far guest's leaving reached the host's session as its stand-in's while the stand-in
   was still in the session (the stand-in was dropped at the radio, "deauth", and never left with the game's
   Disconnect; the host never said "<name> is going home."); and on the guest, the stand-in host's answers to the
   guest's own Disconnect (types 0x04 on) were written over with the far host's movement ("Unable to disconnect.",
   "Unable to receive data."). Now the far player's session steps are not put in, and the stand-in's own session
   steps are not written over: they stay between each player and its stand-in, as the session's messages 0x2B-0x32
   already did. (The new day's Disconnect, the host's, had worked: the host's own Disconnect runs against its own
   stand-in on its own PC.) Verified: the guest's Disconnect ("Disconnecting.", "Disconnected.", "Saving.", "Save
   complete.") and the new day's reconnect (S3b); the host's "<name> is going home." is still to be seen (Still open).
5. **The Friendship / Declare love test cast** (`MQ_COOP_CAST_FILE`, test switch only): it shows the other student
   casting (+0xA2 = 27/28) and never put it back, so that student then ignored its movement on this console (seen:
   the host showed the guest standing in front of the school while the guest had walked away). Put back to standing
   at the end.

6. **The host's side of a guest's Disconnect** (verified, no change needed beyond 4 and the scene fix 3): with the
   host free (no presses on it), the guest's Disconnect ("Disconnect?", "Disconnecting.", "Disconnected.", "Saving.",
   "Save complete.") showed on the host "<name> is going home." (the session's message 202, then 203) and the host
   stayed in its session and walked on; over the internet mode (its stand-in for the guest left with the game's
   Disconnect as the far guest's role went to none) and over the same-PC link (`nd.py disconnect`, logs
   `coop session ... box 202`).
7. **The new day while a player has something open** (`game/src/coop_session.cpp`, `game/src/coop_net.cpp`). The host
   now asks first (message 41, "get free") and the guest's console closes its text box (B pages it), menu (down to
   the last option, then A) or other screen, says it is free, and only then the host's Disconnect runs; the players'
   own buttons are left out meanwhile. If the visit still splits over the internet (the host home, the guest's
   stand-in host still in the old session), the guest finds its host out of a session for 3 s, reports itself out
   (no "playing together" on either window), its stand-in host leaves (the game's "The person you were playing with
   has disconnected."), and it goes home and connects again. Verified at 3:58-4:00 with the guest in a text box and
   in a menu, over the internet mode (also with the guest ignoring the host's message, to see the split mended) and
   over the same-PC link with the host's box open too (it waits until the host's player closes it); docs/COOP.md,
   "When a player has something open at the new day".
8. **Talks piling up** (`game/src/coop_talk.cpp`, `game/src/coop_content.cpp`): one talk at a time between the two
   students; while the other student's talk with this one is on, A in front of them starts none, "came over to talk"
   shows once, and two talks started in the same moment resolve to the host's (the guest's says "<name> is talking
   to you." and ends). docs/COOP_RELATIONSHIPS.md, "Risks".

9. **The farewell's leaving classmate differed between the consoles** (`game/src/coop_class.cpp`): homeroom's people
   (query 0x0C) were each console's own picks and rolls. Now the host's are sent and the guest's query runs on them.
   Verified (same-PC link, the week's S0 saves at 08:40 on 2 May, the host first, `/home/claude/run/live/hr2.py`):
   "homeroom 551's people 72, 52, 68: sent to the guests" on the host, "homeroom 551 takes the host's people 72, 52,
   68" on the guest, the same t131-t133 on both through the swap (72 52 68, 72 52 72, 68 52 72), and Eric (72, g1062)
   said goodbye on both consoles ("I guess it's time to say...", "That's all for homeroom!"). Before, in the same
   set-up: Marty on the host, Eric on the guest. A second cause showed when one console's homeroom began alone (the
   other student not yet in the school) and the other's began during it: the first console's t131 and t133 came back
   swapped (cause not found), Tony instead of Eric; now put back once the scene's swap was done. Verified with the
   guest first (its homeroom alone, "no people from the host after 900 frames: its own", then "t131 and t133 came back
   swapped: the story's classmate 72 is t133 again"): Eric on both. The internet mode was not verified for this one
   (two tries did not get the students to Principal Sol on a loaded machine); the message is the same side channel.

10. **Real input: the tea party and Mr. Graves' tournament** (2026-10-26, same-PC link, the pad on both students;
   docs/COOP.md "Festivals and holidays together" and "The evaluation and the magic tournament together", screenshots
   `/home/claude/run/live/shots/tp7/`). Two failures, both fixed in `game/src/coop_myst.cpp`:
   - *The second student's talk to Miss Strawe was refused while the first one's party ran*: her talk starts 500.3 with
     a request 55 of kind 0, and the host's arbitration (ov001 0x020F0904 -> 0x020F0EDC) refuses kind 0 while another
     console holds the event (`answering ... 32 01 ff 00 f4 01 03 00` five times; the guest's talk closed after
     "You're always working so hard,"). The same class as the walk-in mysteries (kinds 2/4). Now allowed for festival
     and mystery scenes and Mr. Graves' 510.3 when another console holds them (`mq_myst_walkin`); verified: "scene 50003
     (kind 0) asked by slot 1: allowed", both students at their own tea party.
   - *The partner asked to go along was not moved* when it stood 12-24 units off in town (the post office, 13 units
     from Miss Strawe): "beside the other student" was logged at once. Now stepped beside (event 90018); verified.
   - Mr. Graves on the 21st by pad: the host's lesson, the gathering, the guest asked and stepped beside, its own talk and
     lesson, both through the rules to the duel between them (the words by the test switch), the verdict, the host's
     tournament over, the guest's round two. No refusal on the way.
   - Seen, not a failure: at each connect the guest's request 55 for the dorm's scene 25341 (kind 3) is refused
     (`32 01 ff 00 fd 00 29 03`); the scene still plays on the guest (it is started without the answer).

## Still open

- **The school's door once refused both players** (S1b, internet mode): after the host had gone in and out, A at
  the door did nothing for either (inside the door's rect, facing it). Not seen again in three later sessions
  (same-PC link and internet mode, in and out several times); the cause was not found. Note for drivers: the door
  needs A after walking into its rect from outside it (`wk.enter_school`).
- **The tea party** was not reached in the week (reached with pokes afterwards, 10 above): on Tuesday 2 May the town's week event was the tea party (g1242 = 124, Miss
  Strawe in the plaza at 9:11) but she was gone after the lesson; on Wednesday g1242 was 94 and Mr. Graves stood
  there instead. The tea party's own flow was verified earlier (docs/COOP.md, "Festivals and holidays together").
- **No mystery was scheduled** in these days (g1000 = 0 on both consoles every session), so a mystery stage was not
  played in this week; the mysteries' flows were verified earlier with pokes. Found afterwards (same-PC link, a
  mystery poked): **walking into a mystery's trigger area started nothing in a session** (the host's arbitration
  refuses every walked-into story scene, kind 4, the DS's own rule); **fixed 2026-10-26** (`mq_myst_walkin`,
  docs/COOP.md "Walking into a mystery's trigger area in a session"): the host walking in starts 121.4 and the guest
  is asked to join, and the same the other way round, verified by pad input (`/home/claude/run/live/shots/v5/hw_*`,
  `gw_*`). Not yet replayed over the internet mode (the request and answer are the game's own session messages,
  which that mode carries).
- **Insects**: none were out where the students stood (the town's insect managers rolled none: kinds 255, the 70 %
  roll or the place and hour); catching was verified earlier (docs/COOP.md, "Insects together").
- **The café** (Limelight) did not open on A at 9:05 (business hours); not retried in the evening.
- **Pressing A near the other student starts our talk**: a driver (or a player) pressing A to page through a box
  that has just closed starts a talk with the other student when facing them (the game's own behaviour with an NPC
  is the same). Eased by 8: not within 45 frames of one of ours ending, and not while the other's talk with this
  one is on.
- **Our talk menu showed the communications room's options** ("Yes / No / friend code / Exit" instead of "Chat /
  Give an item / How are we getting along? / Never mind": the options work by position, Chat is the first). Seen in
  the week's screenshots (`s3_02_chat_2.png`). **Fixed (2026-10-26)**, any menu on the bottom screen in a session with
  3D on both screens (ours and the game's, e.g. our Gateway menu). Not the text: each option's label is drawn right
  into its canvas (main 0x0200E498 -> 0x0200A63C -> glyphs main 0x02081ABC) and uploaded every frame (main 0x0200A970
  -> 0x02005970: with 3D on both screens, byte 0x020ACA54, an upload for engine 1 goes to engine 0 on parity-1 frames
  and is dropped on parity-0 ones, since the main engine draws both screens in turn and the sub engine only shows the
  capture through 12 bitmap OBJs). The upload's offset is the label's character name times its engine's OBJ boundary
  (the OAM managers at *0x020AD3A8 + engine * 0x524, mode word +0x520): engine 1's was the communications room's 128
  bytes, the main engine's is 64, so each label went to twice its offset in the main engine's OBJ VRAM and the OAM's
  tiles (e.g. name 0xC2 at 0x3080, uploaded to 0x6100) kept what the room had put there. `mq_dual_obj_mode`
  (game/src/cfie_hooks.cpp, end of each frame) gives engine 1's manager the main engine's boundary while 3D on both
  screens is on in a session and puts the game's value back after (docs/COOP.md, "3D on both screens in a session").
  Verified (pair of explore.sav / game.sav, Connect now, `NTR_DRAW_EVERY=0`, outside the dorm): the host's talk menu
  "Chat / Give an item / How are we getting along? / Never mind", the cursor moving down to the third, the guest's
  own talk menu the same, the town on the top screen of both consoles. QUICK PASS.
  **The Gateway's menus with the fix (2026-10-26, release build b642400, pair of day1done.sav copies, Connect now,
  `--clock 2000-07-18T14:00`, `NTR_DRAW_EVERY=0`)**: the host's Gateway (event 25001 started from the event file
  after s411 = 0 and the day's Anywhere flag poked: var 1262, 0x020AD7FE and field+0x3EB0) showed our
  "communications room / Anywhere / Exit" (`/home/claude/run/live/fqh/gw2_menu.png`); Anywhere by pad (Down, A); the
  guest, by pad, got "AAAAAA is going through the Gateway of the Beginning and End!" (`fqh/gw3_asked.png`), then
  its own **"Go with AAAAAA?" with "Yes / No"** on its bottom screen (`fqh/gw3_options.png`), A on Yes, and both
  stood on the beach side by side (map 64, (5, 14) and (6, 14): `fqh/gw3_after.png`). Seen beside it, not faults
  of the menu: upstairs in the dorm the guest's top screen is black (the known top camera there); on the beach the
  host had the Anywhere evening sky and the guest a day sky (the test poked field+0x3EB0 and 0x020AD7FE on the host
  only; on a real Anywhere day each console copies g1262 at its own load, so not judged from this run).
- **A guest fishing in a session (2026-10-26, same pair, real pad input; no catch reached)**. After the Gateway both
  were moved to the bookshop (`e 243` in the warp file; no crash) and the guest walked to the river east of it
  (about (40.2, 73)), took its rod with R R and cast with X held (40 frames: 1-2 units, too short; 100 frames: the
  lure at (43.30, 70.64), a unit from the host's fish 6 and 7). Logs `MQ_COOP_FISH_LOG`, `MQ_FISH_TRACE`,
  `MQ_MYST_ARB_LOG`, `MQ_NET_LOG` (`/home/claude/run/live/fqh/log.txt`, `fqg/log.txt`):
  - Asking worked each time: guest "our lure found fish 6 (1.34 units): asked the host" -> host "slot 1 asks for
    fish 6: yes, it is theirs to run" -> guest "fish 6 is ours (the host says yes)", "the lure's trigger again",
    "a fish from slot 6 goes for our lure" (the hooked fish, state 2 -> 3 -> 1). It left without a bite (hooked 0
    about 420 trace frames later) and on reeling in "fish 6 given back (point 24, at 43.11, 69.53)" -> host "fish 6
    is ours again (point 24 -> 24, at 43.11, 69.53)": both consoles at the same place and point.
  - The second hook (fish 7, asked and granted the same way while reeling) reached state 5 at the lure (the bite:
    the rod bent, a ring on the water, `fqh/fish_bite.png`); the X presses came too late and the game said "The fish
    got away. Reel them ..." (the escape: the game's own flow, ended without a hang); then "fish 7 given back (point
    25, at 44.29, 70.97)" -> host "fish 7 is ours again (point 25 -> 25, ...)". No refusal, no fatal, no stall
    in either log; the host's fish manager ran on.
  - **(A catch is in the next item.) Not seen in this run: a catch in a session** (the strike needs X within a few frames of state 5; a driver must press
    it on the trace's change, not after a shot), so a fish moved to a new point on both consoles by a catch and the
    bag/book writes on the catching console remain to verify. A third cast in the run did not go out (the lure stayed
    at the student after a reel-in loop; a driver timing issue, not looked into). Driver notes: the student cannot
    walk while the lure is out; reel in with X taps until the trace says lure 0; hold X about 100 frames for a cast
    that reaches the river's middle from its west bank.
- **Fish caught in a session, by both (2026-10-26, pad input; one bug found and fixed)**. Same save and clock, Connect
  now, both at the bookshop (`e 243`). A driver that reads the fish trace and presses X like a player
  (`/home/claude/run/live/fishdrv.py`, outside the repo: tap X until a fish is hooked, faster while it comes, never
  while it pulls, which is what lost the earlier two: X during a pull counts toward the line's escape; tap while it can
  be reeled). The guest, west bank at (40.15, 72.6): fish 2 asked for and given, bitten, two pulls and two reels, "You
  caught a Colored Carp!" / "Very big" on the guest only (`/home/claude/run/live/shots/fish2/g3_caught_pair.png`). The
  host, further south at (40.73, 92.31): fish 0, "You caught a Loach!" / "Small" on the host only
  (`shots/fish2/h4_caught_pair.png`). Each catch went into the catcher's bag only (RAM of both). **The bug**: after the
  guest's catch the fish stayed at its old point on both consoles (the guest gave the slot back before the catch's
  message closed, so the game's move happened on a copy the host then overwrote; and the host freed no point). Fixed
  in 9cb5852 (docs/COOP.md, "Fish together"); after the fix the guest's caught fish 2 moved from point 8 to point 11
  and the host's fish 0 from point 0 to point 2, the same on both consoles, old points free and new ones taken. Both
  students casting at one fish: the guest's lure got it, the host's never hooked it (`shots/fish2/both_caught.png`).
  Seen beside it, not fixed: with 3D on both screens the FISH panel shows on the top screen and the catcher's bottom
  screen is sky only (with `MQ_COOP_DUAL_3D=0` the panel is on the bottom screen as alone).
- **Phantasm (2026-10-26)**: its only place is the dorm's washrooms (maps 4 and 5). Lifted and cast there in a
  session, the scene's map reload first ended the event (the load's run slept on sending each command to the other
  console and the event's own thread stepped past it), leaving the student behind a white screen
  (`/home/claude/run/live/shots/fish2/ph_5.png`). Fixed (docs/COOP.md, "Phantasm in a session"): the guest's and the
  host's casts both meet the fortune-teller and end free to move (`shots/phantasm/p2g_1_teller.png`,
  `p2h_1_teller.png`), the other console told; a guest's cast changes only its own record.
- **The bank's safe deposit (2026-10-26, by pad)**: the host's George shows the normal menu and its safe deposit
  works while connected (a wardrobe dragged in with the stylus, OK, "Thank you."; `shots/bank/h_dragged2.png`); the
  guest's Use safe deposit answers "I'm sorry, but you can't use that service while visiting another town."
  (`shots/bank/g_refused.png`): its safe deposit is in its own town, not loaded during the visit. Left so.
- **A silence of 5 s** (a debugger pause here) makes a player's stand-in leave the session ("the far player left");
  the leave met "Unable to disconnect." and the stand-in stayed, so the session survived; a real 5 s network outage
  would end the far player's visit on that PC.
- **The guest's clock** runs a few minutes off the host's in unpaced runs (docs/COOP_INTERNET.md, Open).
- **A game closed right after a visit's error** ("Unable to receive data", then turned off within seconds) loads
  into the warden's punishment next time: the game's own rule (docs/COOP.md, "Turning off without saving").

## Each player's progress after the week

Each player's last saves (the host's from S3b's connect after the new day, the guest's from its Disconnect after the
next day's lesson) loaded alone, without co-op, `--clock 2000-05-05T10:00` (RAM, `.coop` beside each save):

| | money | bag | s411 | s227 / s228 | town date | pair record (.coop) |
|---|---|---|---|---|---|---|
| host | 3000 | Beige Wardrobe | 0 | 0 / 6 | 5 May | stage 0, friendship 4, affection 1 |
| guest | 3000 | Beige Wardrobe | 0 | 1 / 10 | 5 May | the same line |

- Each kept its own record: the guest's lessons (S1f, S3b) are in its own save (s227/s228 higher than the host's,
  whose lessons of S1f and S4 were never saved: it was closed during those visits, and a visit saves only at its start
  and end, the game's rule); `s411 = 0` (the test poke) was kept by both; the pair records are equal on both PCs.
- The host's save loaded straight into its dorm (the visit marker: "not punished"); the guest's save, written by its
  Disconnect and closed a few seconds later at home, loaded with the warden's "You can't turn off the power..."
  (the game's own rule for a game closed without saving outside a visit; docs/COOP.md "Turning off without saving").
- S1a's gift (the Beige Wardrobe to the guest) and S3a's chat were in sessions that were later played again from the
  earlier saves, so they are not in these saves.
