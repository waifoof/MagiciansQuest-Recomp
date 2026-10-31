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

11. **Real input: daily life** (2026-10-26, same-PC link, two day1done.sav copies with visit markers, Connect now,
   the pad (and the touch screen where the game wants it) on both students; harness outside the repo:
   `/home/claude/run/live/dlpair.sh`, `dllib.py`, `dl_*.py`; screenshots `/home/claude/run/live/shots/dl/`;
   `MQ_MYST_ARB_LOG=1` on both). Per step:
   - *Talk and romance up to the invitation and a hang-out*: **works**. The pair record started at friends 62 / 40
     (the `.coop` line, no grinding); A on the other student: "Oh, hi" and on the other console "came over to talk"
     (`t1_talk.png`); "There's something you've been..." and *Meet at Lovers Point?*, the time menu (`t1_menu.png`),
     10 a.m.; the other console's invitation in the asker's voice with *Sure, I'll be there!* (`t1_invite.png`), A:
     "looks so happy! See you at Lovers Point." (`t1_answer.png`); both `.coop` lines: invitation 2, time 1. Both
     moved to Lovers Point by `MQ_WARP_FILE` (`49 6.2 3` / `49 7.2 4`: the driver did not find Lovers Point's way in
     from the town); at 10:05 the scene started on both by itself ("came to Lovers Point, right on time" /
     "is waiting for you", `lp_now.png`), the confession, the other's *Let's be a couple!* (`lp_choice.png`), "smiles
     and says yes!" / "beams at you" (`lp_result.png`), both records stage 4; then the couple's five-option menu
     (`hang_menu.png`), *Hang out*: "Your heart feels warm." / "You spent a lovely time with", +3 / +2 on both copies.
     Driver note, not a fault: an A pressed to page a box that has just closed starts a new talk with the other
     student when facing them (the known "Pressing A near the other student" below).
   - *Giving an item*: **works both ways**. Give an item, the bag on the touch screen, the Beige Wardrobe dragged out
     of the first slot (touch held 10 frames, then moved off the bag): "You gave the..." / "You put it in your bag."
     (`gift_after.png`), then the guest gave one back the same way; RAM: one wardrobe in each bag, `.coop` gifts 1 on
     both. Seen: a drag from the bag's *second* slot only showed the item's name (a driver's coordinates, not looked
     into); with 3D on both screens **the bag's item name and Principal Sol's menu title are drawn on the top screen**
     (red "Beige Wardrobe" over the sky, blue "Introduction" over Sol; their white label bars on the bottom screen
     stay empty: `gift_back.png`, `g_sol_menu.png`). **Fixed** (docs/COOP.md, "3D on both screens in a session":
     the field's labels were set up for the main engine in a session; `mq_dual_field_frame`).
   - *A lesson together, walking into the school*: **works** (after the fix below). Both by pad from the school's
     arrival point: the door (A), upstairs, Principal Sol (A), "So, which class...", Staffs, the lesson; side by side
     at the desks with the same classmates in front on both consoles (`les3_mid.png`), both to "Well, that should just
     about...". The first student's homeroom or lesson starts alone when the other one is still outside the school
     (the rule of "Lessons together": it waits for a student in the school or with a lesson picked); the second one's
     starts with "the other student: in class".
   - *Homeroom (the farewell)*: **bug, fixed** (`mq_coop_remote_cmd`, game/src/coop_myst.cpp). The host's farewell was
     said by Tony, the guest's by Eric (`hr_end` texts). Cause, found with the arbitration log and gdb on the
     copy_var handler (ov028 0x021135D4): in a session the interpreter sends some commands to the host (request 0x39:
     copy_var 0x5B, set_student 0x3B) and *every* console runs the answer for the console that sent it (ov023
     0x02110A68 / 0x0211068C -> main 0x0201C770 with the player = that slot). On the DS no scene runs on two consoles
     at once; here each runs its own, so the guest's homeroom swap "t133 = t131" rewrote the host's t133 in the middle
     of the host's farewell (and the host's the guest's: the "t131 and t133 came back swapped" of 9 above, whose
     cause was not found then). Now another console's writes of this console's script bytes (variables 10000-19999:
     copy_var into them, set_local, add_local) are not run; its record's and the town's are. Verified twice (host
     first, guest first): "slot 1's command 5b writing t133 not run here" on the host, "slot 0's ... t131 / t133 ..."
     on the guest, t128-t133 unchanged, Eric on both (`hr3_end.png`), and the lesson after it the same on both.
     `MQ_COOP_REMOTE_LOCALS=1`: the game's way (research). The same writes reached the other console in every scene
     played on both at once (lessons' seat swaps, Sol's menu, mysteries and festivals' copy_vars), so this may mend
     other "classmate differs" cases too.
   - *Talking to a classmate the other student is talking with*: **bug, fixed** (`mq_coop_npc_talk_ok`). The guest's A
     at Brandy (79) while the host talked with her gave nothing: the host answered the guest's request 53 (0x35, talk
     to NPC) with 50 three times (`32 01 ff 00 21 8e ...`): ov001 0x020F08D4 refuses an NPC another console talks with
     (0x020F0EDC kind 1). Each console runs its own copy of the classmate, so a talk taken by another console is now
     allowed (not recorded; a free one is the game's). Verified: "slot 1's talk to NPC 79 allowed", both reading
     Brandy's "Nice to meet you..." at once (`npc3_both.png`), both talks to their ends. `MQ_COOP_NPC_TALK_GAME=1`: the
     game's refusal.
   - *The new day*: **works**. Both free in the host's dorm at 3:50 (2000-05-03): at 4:00 "the host's Disconnect for
     the new day", "Save complete." on both, both home, both connected again into the host's dorm at 4:01
     (`nd_after.png`). The game has no bed or sleep action (the day turns at the season's hour, "How a visit ends"), so
     "going to bed" is standing in the dorm until then.
   - *Casting by pad*: Y opens the incantation's words (two screens, `cast_g_words.png`); the words are tiles touched
     twice (Friendship: "all the time" on BASIC, "together" on the third type, DO; their ids from the charm table and
     `/picstat/magic/msg_e.dat` / `order.dat`). **OK crashed both games** ("fatal: jump from 0211e91c to
     00000000"), and so did closing the words with X, **in single player too** (day1done.sav alone, Y then X, with or
     without words picked). **Fixed** (e51be57, runtime/src/runtime.cpp `interpret`): a recompilation fault, in every
     build since the bring-up (the code dates from the first commit; a build of 14:22 the same day crashed the same
     way). Cause: the words screen's close (ov034 0x0211E8DC) calls the thunk ov001 0x020FAD9C (`r0 += 0x984; bx
     main 0x0204837C`), which is no recompiled function and runs in the interpreter. The interpreter runs the
     tail-called recompiled function natively, and when it came back to the interpreter's own return address it
     stored melonDS's registers into the CPU state: the registers from before the call (r0 lost), and here another
     thread's, because 0x0204837C waits (main 0x02003BFC, the task yield) for the words task (thread 39, ov001
     0x020F73C1) to end, and the other threads' interpreted code (the task exit 0x02003C60 and others) loads melonDS's
     registers meanwhile. So thread 38 went on with thread 39's stack pointer and popped 0 as its pc. Now that return
     keeps the CPU state the native function left. The four ov001 functions added to functions.txt before (6a935eb)
     were not the cause (harmless). Same pattern, all now safe: the words screen's other ov001 thunks
     (0x020FAF94, 0x020FAF64, 0x020FAC74, 0x020FAF48, 0x020F1740) run in the interpreter too.
     Verified with the fixed build (screenshots in `/home/claude/run/live/shots/wfx/`):
     - Solo (day1done.sav, then the sweep's host save) and the pair's guest: Y, words, OK (the words drawn above the
       student, no crash; `pg_m0_words.png`, `pair_m0_ok.png`) and X (`pair_m0_x.png`), several times, walking on
       after each. On these saves the student's flag
       510 is clear (record bit at +0x635 bit 6, main 0x0201DFC0; read by ov001 0x020CAD14 when the words open):
       the words screen then opens in its mode 0 (main 0x0204837C's first path, the one that crashed), whose OK only
       drafts the words (shown above the student; result 0 to main 0x02022A5C), nothing is cast. No test save of
       ours has the flag (checked: day1done, explore, game, the playtest saves); what sets it in play was not found
       (it also reads 0 again after a map change).
     - With the flag set (`var 510=1` by `MQ_COOP_POKE_FILE`; the hand tool shows as the wand), the words open in
       mode 2 and OK casts at once (result 2: main 0x02046F98 -> 0x02047A6C -> 0x02047A10, the result applied by
       main 0x02045500): "incantation incantation" (charm record 40, result 68) cast in town with the sparkles
       (`m2_a.png`), the words gone and the student free (`m2_c.png`); X after picking a word closes cleanly
       (`m2_xclosed.png`).
     - Connected pair (`/home/claude/run/live/wfxpair.sh`, the sweep's saves, own link): both students facing each
       other in the host's dorm, Friendship composed and OK on both at once: both cast (`p_fr1.png`), no crash; the
       guest's cast shows its thought bubble on the host's console (`p_inc1.png`); a guest's OK and X in mode 0
       (`pair_m0_x.png`, walking on after). Friendship from the words lands in the charm table's catch-all record 42
       (event 382) on both: the OK path casts with no target (-1, main 0x02046F98), and the other student is never a
       target (the talk finder's NPC list, docs/COOP_RELATIONSHIPS.md); facing a classmate (NPC 65) gives the same.
       How the game aims a targeted incantation by pad (main 0x02046E90 with a kind, gated by *0x020B8E26) was not
       found; Friendship together still works by `MQ_COOP_CAST_FILE` ("Your Friendship incantation reached
       <name>!", `cast_file2.png`).
   - *Casting by real input* (2026-10-27, research and fix; recipe: docs/TESTING.md, "Casting an incantation by real
     input"). **Flag 510 is the wizard form**: a player sets it by **turning the D-pad (or the stylus) clockwise**
     (action 30, main 0x02025508 writes 510 at 0x02025620; the lessons: "you can transform by using the stylus or
     +Control Pad to make yourself spin to the right"); the tip "Transform into a wizard!" and then the words screen
     open by themselves; counter-clockwise turns back; a map change ends it. The harness never turned, so the words
     only drafted. **Targeting**: an incantation is aimed by its record, not by facing: the record wants a place
     (Friendship: the ruins, town zone 32), the words (Friendship: *incantation*, *all the time*, *together*: the
     earlier tests left out *incantation*, which every incantation begins with, so they hit the catch-all 382), then
     an action with an item (Friendship: hold the emerald out of the bag, kind 14), and a target: alone, a classmate
     going about with the student (hang out: *0x020B8EF4, query 0x37); in a session, the other console's student
     who has transformed (query 0xB5, packet 0x45: that student joins with action 27). Facing a person does nothing.
     - Alone (day1done.sav, the ruins, emerald poked into the bag; screenshots `/home/claude/run/live/cs1/`):
       transformed (`spin2.png`), the words with *incantation* chanted (action 23, the 90 s running), the emerald held
       (`t3.png`): the cast (kind 14, item 428, Friendship's record) and its event, failing in main 0x020474F0 (no
       classmate with the student, seen with gdb on the checks): "You failed at using your incantation." (event 320
       block 2). A classmate's hang-out was reached (Eric's "You wanna do something together?", *0x020B8EF4 = his
       object) but ended before the walk to the ruins was done; a cast on a classmate by real input was not reached.
     - Connected pair (`castpair.sh`, the sweep's saves, both at the ruins; screenshots
       `/home/claude/run/live/shots/cast/`): both holding the emerald at once failed on both (each in its own event
       when the other asked). **The guest transformed, the host said the words and held the emerald**: the host's
       0xB5 passed, the guest joined (action 27), "You succeeded in using the Friendship incantation!" and "Your
       Friendship incantation reached AAAAAA!" on the host, friendship +2 on both copies (`fr1_4.png` .. `fr1_7.png`,
       `/home/claude/run/live/cph/c_fr3.png`).
     - **Bug, fixed** (`game/src/coop_talk.cpp`): the joining console's "<name> cast the Friendship incantation with
       you!" (our 90004) was queued the frame the cast message came, while its student was in action 27, whose step
       20 queues the game's own partner event (320 block 4): that one was never queued, and the student stood
       chanting, unable to move, until the words' 90 s ran out (event 369 block 3, `c_fr5.png`). Our pending events
       now wait while the student's action is 27 or 28. Verified: "You were only slightly excited." (320.4), then
       ours, the student free after. QUICK PASS.

12. **Real input, part 2** (2026-10-27, same-PC link, two copies of a save with visit markers, Connect now, each
   feature reached by pad and touch; set-up (date, variables, items) by pokes; harness `/home/claude/run/live/sw2pair.sh`,
   `sw2lib.py`, `sw2_words.py`, `sw2_gate.py`, `sw2_gate2.py`, outside the repo; screenshots
   `/home/claude/run/live/shots/sw2/`):
   - *The tournament's duel, the words drawn with the stylus*: **works**. day1done.sav, `--clock 2000-05-21T15:30`,
     `var 411=0`, `20ad7fe=1`, four tournament incantations (s298-s300, s303) poked on both, both into town by `e 239`.
     The host by Mr. Graves, A, "Extracurricular lesson" (`t3_menu.png`), 521.1; the guest asked "Go along with
     AAAAAA?" (`t5_guest_asked.png`); the driver's Yes came after the offer's minute and it heard "Looks like AAAAAA has
     already worked it out." (the mysteries' line for an offer that is gone; see the offers' note below), then talked
     to Mr. Graves itself and entered. Both through the rules by A: "duel: the first round is
     against the other student", the host's draw 8 (s303, party popper magic). The words open on both
     (`t8_words_open.png`); the host tapped *magic* (BASIC), *flower* and *instrument* (WHAT, page 1) with the stylus
     (`t9_host_words.png`) and OK: "the right words in 6861 frames", "duel verdict: slot 0 wins", the party popper on
     the guest (`t11_after_ok.png`), "And the winner is... AAAAAA!" on both (`t12_later.png`); the guest's tournament
     ended in the hall as the game has it, the host on to round two (sleep magic: *magic*, *daytime* (WHEN),
     *sleepy*), lost to the classmate, "Well done, students!", both in the hall (`t20_guest_end.png`). The winner's
     words panel stays on the bottom screen after the round: the game's own (no 0xB2 on the winner's branch).
     The words screen: six tabs (x 13, 37, 61, 84, 107, 130 at y 178), 3 rows x 5 per page, a page arrow at (246, 94),
     ids in `/picstat/magic/order.dat` order (BASIC from 0, WHO 30, DO 165, WHAT 195, WHERE 240, WHEN 285); the
     tournament's twelve are the charm records 20-31 ("magic" + two words).
   - *The Gateway to "The other world" and back, the partner along*: **two bugs, fixed**. day1done.sav,
     `--clock 2000-05-23T10:00`, on the host `var 411=0`, g1000 = 32, g1001 = 2, g1010 = 1, g1011 = 0 and item 2106 in
     the bag. The host pressed A at the Gateway: our "communications room / The other world / Exit"
     (`g2_host_menu.png`), Down, A: the walk in, map 38; the guest asked "Go with AAAAAA?", Yes: beside it in map 38
     (`g5_both_otherworld.png`).
     1. Map 38 has a door: the tower's (map 59), where mystery 32's scene 102.3 runs. The host walking into the tower
        had the guest told "You go back with AAAAAA." (`g8_tower.png`), as if it had left the other world (and the
        guest, slow to page the note, then stayed: "too late"). Fixed (`game/src/coop_gate.cpp`): the other world is
        maps 38 and 59 for both students ("map 38 -> 59, still in the other world"); the note's warp no longer times
        out while the note waits for A.
     2. In the tower, 102.3 waited for ever at the principal's first line ("Oh, there you are,", the box's name tab
        only: `g11b_host.png`): he (actor 121) had no model. In a session a building's placement gives no one a
        model and the visitors' call put the old man (212) in model 4; 102.20 registers the principal later. Back in
        the dorm, 102.4 waited the same way at his line. Fixed (`game/src/coop_class.cpp`, `scene_people_models`,
        once a frame, no regeneration): in the tower, and in the dorm while one of mystery 32's scenes runs, each
        person (not a classmate) without a model gets a free model (not another player's), place kept; the old man
        gets a free one instead of model 4. A first, wider version (any building while any scene ran) gave the dorm's
        warden the guest's model slot during the connection and hung both consoles: kept to these scenes.
     Verified after the fixes: host and guest each walked into the tower by its door (A at it); the old man and the
     principal with models on both consoles; the host talked to the old man, the guest, standing near, joined the
     scene ("near the other student: joining scene 10203 at once"); the principal walked in and spoke
     (`g22_principal_1.png`), both consoles through 102.3 to the dorm, 102.4 with the principal ("What a relief...",
     g1001 = 4) on both (`g25_dorm_scene.png`), both free in the dorm (`g26_free_dorm.png`). QUICK PASS.
     Not compared with single player in the tower (a solo copy's map load clears a poked mystery, and its tower door
     led straight back to the dorm).
   - *Catching an insect while the other player is near it*: **bug, fixed; then works**. day1done.sav,
     `--clock 2000-07-18T14:20` (the clock seeds the rolls: a fresh pair until a ground butterfly sat by the flower bed
     west of the bookshop, (19.4, 65.5); `/home/claude/run/live/sw2_insgo0.sh`), both by the bookshop (`e 243`), the
     net taken with R.
     - **The bug**: in a session a butterfly a student walked up to was lost at once: its place (+0xFC) became (0, y, 0)
       about 8 units away and stayed so; never drawn again (the base update, ov001 0x020DD280, thinks and shows an
       insect only within 10 units of its watched student, now measured from (0, 0)) and never hit by a net. The same
       with `MQ_COOP_INS_OFF=1` on the host (the game's code in a session); alone the same walk left it fluttering by
       its flower. Found with a write watchpoint on +0xFC (gdb, attached once): the write is the base update's move
       through the town's collision (field+0x3EB8's +0xC, ov002 0x02103E30, from ov001 0x020DD36E) from the insect's
       last place +0x108 to its new place. +0x108 is 0 until an insect's first move (alone too: RAM of a solo copy);
       alone that first move from (0, 0) comes out right, in a session it ends at (0, 0) (why the collision differs in
       a session was not found). **Fixed** (`game/src/coop_insects.cpp`, `mq_ins_update`, only in a session, no
       regeneration): an insect whose last place is still 0 gets its place as its last place ("insect n's last place
       set to its place"; `MQ_COOP_INS_PREV_GAME=1` as the game has it). After it the butterfly flutters by its flower
       on the host as alone (its +0xFC within 2-3 units of it while the student walks up).
     - **Catches by pad after the fix**: the host walked up (the guest 13 units away) and swung with X: "You caught a
       Small Cabbage White!" / "Quite big", the guest's copy gone ("insect 0 gone at the host"). A fresh pair: the
       host standing about 4 units from the butterfly, the guest walked up and swung: guest "our net caught insect 0:
       asked the host", host "slot 1 asks for insect 0: yes, it is theirs (removed here)", guest "insect 0 is ours (the
       host says yes): caught", "You caught a Small Cabbage White!" on the guest with the INSECTS panel, the host
       beside it on both consoles (`i49_guest_caught.png`). Driver notes (`sw2_ins.py`, `pad_catch`): with the net in
       hand a touch on the bottom screen swings it (action 4) as X does; a touch on an insect walks there first; X 1
       unit from the insect's +0xFC (not +0x114, which is only where it is drawn) facing it catches.
     - Seen beside it, a test tool's fault, not real play: after the host's catch, a guest moved by `MQ_WARP_FILE`
       (`e 240`) hung both consoles (the guest in the school's front without its student or icons, the host's game
       frozen; the same as the note at the end of this file); the guest walking into the bookshop by its door after
       a host's catch did not (both walked on).
   - *The weekly evaluation with a rank-up, both evaluated*: **bug, fixed**. day1done.sav, `--clock 2000-05-03T15:30`,
     `var 196=0`, `var 507=1` poked on both, `MQ_HALL_SCORE=12` (the score that ranks up). The host was put in the
     auditorium (`8 8 9` in the warp file; the hall's door to it was not found in the time); 590.1 started by its
     record; the guest was asked "AAAAAA is being evaluated in the auditorium! Go along with AAAAAA?"
     (`e8_eval_start.png`; the driver's Yes again came after the offer's minute: "Looks like ...", and it was put in
     the auditorium the same way, where its own 590.1 started). Both evaluated on their own records by A, each its
     own rank and hat ("In recognition of your effort...", "I also present you with the..."), then **both stuck** at
     590.1 #634 for ever, the staff roll's poll (query 0xA1) answering "not over" every frame
     (`e12_stuck.png`: both students on the stage in the new hat). Cause: in a session we pass over 0xA0 (the roll
     can't load: overlay 25 shares overlay 23's RAM), and 0xA1's own answer (ov028 0x021196D4) reads the roll's object
     at *0x02110060, inside overlay 23 in a session: that word was 0x18200192 on both consoles, so the game took it for
     a roll still playing (the earlier test passed with that word 0). Fixed (`game/src/coop_hall.cpp`): in a session
     0xA1 answers "over" (t0 = 1, t1 = 255, the game's own answer with no roll loaded). Verified (fresh pair, by A):
     both through #634 ("the staff roll's poll answers \"over\""), "Keep up the good work!", "You've improved
     your...", both in the school's hall, "The staff roll will play once you're back home." and the `.staffroll`
     marker on each (`e16_hall.png`); the host's Disconnect (Select, Yes) took both home ("Save complete." on both),
     and on each console "to the auditorium for the staff roll the session owed", "plays now" (event 90042, about
     15000 frames), then both in the school's hall, the markers gone (`e19_roll_home.png`). QUICK PASS.
   - *The offers to go along* ("<name> is being evaluated in the auditorium! Go along with <name>?", the tournament's,
     the mysteries'): the late Yes above was the driver's (the question's first box waits for A, and the Yes/No menu
     takes A only about a second after it opens). Changed (`game/src/coop_myst.cpp`): the minute to answer now counts
     from the question, not from the other student's scene start, so a student busy when it began (in a talk, a
     menu) is not left with less. Verified (pair, `--clock 2000-05-03T15:30`, s196 = 0, s507 = 1): the guest in our
     player talk for 45 s while the host's evaluation began, then the question, Yes: "yes, joining scene 59001", its
     own evaluation in the auditorium ("Now it's time to evaluate"). With a prompt Yes and no talk: joined the same.
     QUICK PASS. The wording for a gone offer ("Looks like <name> has already worked it out.") is the mysteries' and
     read oddly for a tournament or an evaluation. **Fixed 2026-10-27** (0046d04): the question's kind (t155) was gone
     by the time a late Yes was answered, so event 90020 fell back to the mysteries' line; it is now written again
     before 90020 is queued, and the auditorium has its own lines: "<name>'s evaluation has already begun." and
     "<name> has already gone in to the magic tournament." / "You can still talk to Mr. Graves to enter it." Seen
     rendered (90020 queued with t155 = 51, 50, 0 by the test files: `/home/claude/run/live/shots/sd/w1_tournament.png`,
     `w_grid.png`); a late Yes by real input was not replayed.
   - *A telegram to the other player*: **sending works; the receipt was not seen** (then; found and fixed below). day1done.sav copies,
     `--clock 2000-05-03T10:30`, both into the post office (`e 239`, the door by pad). The host at Thom, "Write a
     telegram": "Telegrams cost 500 Ritch...", "Who are you writing the telegram to?", the menu "Other player / Exit"
     (`m8_host_other.png`; while the guest stood at the counter too the host's list once showed only "Exit",
     `m4_who.png`: the game's own list, ov009 0x0210B04C, from the session's players). The guest, by pad and touch:
     Thom, "Write a telegram", "Other player", the host's name "AAAAAA" (`m16_guest_players.png`), "So you want to
     send a telegram to AAAAAA?", the keyboard "To: AAAAAA / From: AAAAAA" (`m17_keyboard.png`), letters typed by
     touch, OK: "Okay! You can count on us to get this telegram delivered...", 500 Ritch taken, Thom's cat off with
     it ("All right then. Deliver this,"). The session's packet 0x42 went to the host and was arbitrated (no
     refusal: "arbitrating ... 42 01 00 00 ..." / "42 01 00 01"); the telegram's text sits in the town block's
     telegram list (entries of 0xE4 bytes, this one at 0x020B6C84, its text at +0xC) on both consoles, and in the
     host's save after the Disconnect. The host's Telegrams tab (Start menu) stayed empty during the session
     (`m22_host_telegrams.png`, also after 100 s), after the Disconnect (`m24_host_home_tel.png`) and with the host's
     save alone on the next day at 10:00 (`m26_nextday_tel.png`; Thom had nothing for it either). How the mail cats
     deliver ("Let me read you your telegram!", bank 32 line 8783, no event script uses it: code) was not found; the
     test used two copies of one save (both students named AAAAAA, both in record slot 0), which may matter for whom
     the list entry is for (its header `87 03 10 fb 00 00 00 06`). Every test save here (day1done.sav,
     explore.sav, game.sav) holds the same student: name AAAAAA and the same identity (`28f966823fcec90a`, the pair
     key in the `.coop` files), so the telegram went from a student to "another" with its own identity; whether the
     mail cats then deliver it at all is open. Next: a second save with a student of its own (a new game), then a few
     days of play, or the delivery code found statically (the list's readers: the town block's entries of 0xE4 bytes
     from 0x020B600C, `+6 = -1` a free entry; Phantasm's op 0x73 adds to the same list).
     **Follow-up (2026-10-27, a second test save with a student of its own, NORA, docs/TESTING.md):** the receipt is
     the **mail cat's**, in the dorm: entering it, 225.2 places the cat (138) by the addressee's room door when the
     town's waiting list holds one for that student (query 0x85), and talking to it (225.3, query 0x8B) hands the
     telegrams over into the TELEGRAMS tab. The earlier test never walked into the dorm. Two co-op bugs: in a session
     the cat had **no model** on the host (never seen, never talked to), and on a **guest** (its student is slot 0 of
     its own town, as the host's) the cat came for the host's telegrams. **Fixed** (`coop_class.cpp`: the cat gets a
     free model in the dorm's halls; `coop_hall.cpp`: on a guest 0x85/0x8B answer none). Verified by pad both ways
     (NORA guest -> AAAAAA host, AAAAAA guest -> NORA host): the cat by the host's door with a model, the telegram read
     and in the tab, in the session (`h_dorm2_cat.png`, `h_tab5.png`, `a2n_text.png` in
     `/home/claude/run/live/shots/tg/`); before the fix, after the Disconnect the host at home alone got it. The
     host is offered no recipient ("Other player", then only "Exit"): the game lists the town's own students only;
     as on the DS. Details: docs/COOP.md, "Telegrams in a session". QUICK PASS.
     With two different students, also by pad: the players' talk ("Oh, hi, AAAAAA!" / "AAAAAA came over to talk with
     you.", Chat: "You feel a little closer to NORA"), the pair record (`<save>.coop`: one line
     `414141414141-28f966823fcec90a 4e4f5241-28f966ba7f3a30cf ...`, the same on both consoles, the same line whichever
     hosts) and Friendship cast together at the ruins (`castpair_fr.py`: "AAAAAA cast the Friendship incantation with
     you!", friendship +2 in both files, `shots/cast/tgfr_4.png`): all as with one student copied; nothing differed.
   - *One more incantation by real input: Rainmaking*: **works**. day1done.sav, `--clock 2000-07-18T14:00`, both in
     front of the school (`e 240`); the Rainmaking record (charm 17: *incantation*, *sun* (WHAT, page 3, row 0 col 4),
     *sad* (BASIC row 1 col 1), the item 426 held, anywhere; result 50, event 334, one of the game's own two-player
     incantations) and item 426 poked into bag slot 1. The guest turned the D-pad clockwise (wizard form, the tip,
     the words screen), tapped the three words (`r6_words.png`), OK (chanting: action 23, the 90 s), then dragged the
     item out of the bag: event 334 on the guest, block 4 on the host, "You succeeded in using the Rainmaking
     incantation!" on **both** consoles (`r9_result.png`), g1100 = 50 and g1101 = 1 in both towns (the guest's writes
     went to the host as requests: "request 1 from slot 1 ... +33c"), and the same sky on both afterwards: kind 0
     with the next kind 3 (rain) and the same weather byte (0x020A2EC0 = 2) (`r10_later.png`). Alone the same cast
     by the same input is refused by the game ("Someone saw you using the..."): Rainmaking is a two-player
     incantation. Driver note: the first touch on a words screen that has just opened is lost (wait about 40 frames
     after it opens, or tap the tab twice).

13. **Real input over the internet mode, at 0 and 200 ms** (2026-10-27; four processes, `NTR_LAN_DELAY=80:40
   NTR_LAN_LOSS=1` on both copies: 205-228 ms round trip, 1 % loss; host AAAAAA from `day1done.sav`, guest NORA from
   `second_day1.sav`; details, the table and the measurements in docs/COOP_INTERNET.md, "Re-check by real input at 0
   and 200 ms"; screenshots `/home/claude/run/live/shots/inet/`). Connect now, chat and gifts both ways, homeroom (the
   farewell) and a lesson, a walk-in mystery (area 10), the new day: **work** as over the same-PC link. Three bugs of
   the internet mode's message rules, all fixed (`game/src/coop_net.cpp`, `coop_sync.cpp`, `coop_class.cpp`):
   - *A host's Friendship failed* ("You failed at using your incantation."; the guest never joined): the host's
     answers 0x2D / 0x2E to its own 0x44 / 0x45 were kept on the guest's PC by the stand-in as "the session's own".
     Now tapped. After: "Your Friendship incantation reached NORA!", the guest's "You were only slightly excited.",
     +2 on both, once; the words' panels over the top screen (left on both consoles by the failed cast) gone too.
   - *A guest's telegram never reached the host*, and the host's session then held the guest's second 0x42 in its
     inbox for ever: every later request of the guest waited behind it, and the host's next field load hung (a black
     screen, twice). Now the far guest's 0x42 is not put in on the host, the entry goes by the town sync (as the
     host's game would have made it), and a request waits at most 300 frames for the inbox. After: the cat by the
     host's door, "HI", "From: NORA".
   - *The guest's tower scene hung* at the principal's "Oh, there you are," (no model for him on the guest: the
     unused slots' remotes held no model, so none counted as free). Now a model no remote holds is free for a
     scene's person once every player has one. After: both through 102.3 into the dorm.
   - *Fishing, a time-out*: the guest's catch went through ("You caught a Colored Carp!") but the host took the fish
     back meanwhile ("no word of the guest": 90 frames against a record resent every 60). The fish's and insects'
     stale and ask limits are now 180 frames; after: the fish given back after the catch, as over the same-PC link.
   Not from the internet link: the guest's student has the host's hair colour on the host's screen in the dorm after
   connecting, until the first map change (the same over the same-PC link; earlier tests had one student only; only
   with 3D on both screens: `MQ_COOP_DUAL_3D=0` draws her right). **Fixed 2026-10-27** (docs/COOP.md, "The guest's
   hair colour on the host's screen"): the game's VRAM upload queue (128 entries) filled while the host's session
   waited for the guest's arrival (the queue is not done then, and with 3D on both screens the bottom screen's 2D is
   queued again about 12 times a frame), so her hair palette, queued when her model is dressed at the end of the
   wait, was dropped; repeated entries are now taken out of the queue. After, by real input (Connect now, same-PC
   link): NORA green on the host's screen, AAAAAA orange on the guest's, in the dorm and in town; every palette of
   both models the same on both consoles (`/home/claude/run/live/shots/hc/final_dorm.png`, `final_town.png`).
   Also seen in this week's checks, not from the internet link: a student who joins a walk-in mystery from the
   dorm's square (the step beside, event 90018) had a top screen of sky only (`shots/v5/hw_5_after.png`). **Fixed
   2026-10-27** (docs/COOP.md, "A joiner's town after the step beside"): the town keeps one district's buildings
   loaded and switches it only where the student walks through a district's zone; the step crossed none, so the
   school was drawn as ground. After, by pad input (either student walking in, the other joining from the dorm's
   square): the school on both top screens (`shots/hc/b2after_5_after.png`, `final_gw_6_later.png`).

## Still open

- ~~**A host's game killed in a visit over the same-PC link** leaves the guest waiting for its frames (no error, no
  save)~~ **fixed 2026-10-27** (f3b09cb, 1c633bb; docs/COOP_INTERNET.md "A player's game gone"): the same-PC link
  finds a copy whose process ended at once (and one silent for 8 s), so the guest gets the game's own "...disconnected
  due to a communication error." / "Unable to receive data.", goes home and plays on; the host whose guest died the
  same at full speed. Checked on every link (same-PC, UDP between PCs, internet mode at 200 ms with 1 % loss) both
  ways, and the killed player started again and connected again each time.
- **A late Yes to the other's tournament or evaluation by real input** (12 above): the lines are fixed and seen
  rendered from test files; the real late path was not replayed.
- **Friendship on a classmate by real input** (11 above): flag 510 (turning clockwise), the words and the emerald
  are found and work alone and in a pair; the solo cast still wants a classmate hanging out with the student at the
  ruins, which the harness did not reach in time (the hang-out ended on the way).
- **With 3D on both screens, the bag's item name and Principal Sol's menu title are drawn on the top screen** (11
  above; cosmetic).
- ~~**The school's door once refused both players**~~ (S1b, internet mode; settled 2026-10-31, "The school's door and
  the dorm's stale copy" below): not the game's rule and not co-op. Since 298977a (2026-10-26) `MQ_POS_FILE` holds the
  student's place, not the bottom camera's point (3.4-7 units further south in town), and the drivers' door point
  (43.9, 27.4) was a camera-point place: from there Up 10 frames ends at z ~26.7, three units short of the door
  (z ~23.8, right at the arrival point 240's (44.0, 23.91)), so A did nothing. Solo at the soak's save and time (Jul 19
  10:00) the door opens from (44.0, 23.85); in a pair over the internet both went in by pad. The drivers (`wk.py`,
  `soak.py`, `soakcm.py`, `soak3.py`) now go to (44.0, 24.6) first. The earlier one-off (2026-10-27, day 1) may
  have been the same stand point. Also seen: a student standing in front of the door gets the talk, not the door.
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
  of the menu: upstairs in the dorm the guest's top screen is black (the game's top camera there, as alone; shows the
  corridor since 2026-10-27, "The top screen place by place" below); on the beach the
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
  Seen beside it: with 3D on both screens the catcher's bottom screen was sky only under the FISH panel. **Fixed**
  (the panel on the top screen is single player's too; the bottom screen now shows the student putting the catch
  away as alone: docs/COOP.md, "Fish together" and "3D on both screens in a session").
- **Phantasm (2026-10-26)**: its only place is the dorm's washrooms (maps 4 and 5). Lifted and cast there in a
  session, the scene's map reload first ended the event (the load's run slept on sending each command to the other
  console and the event's own thread stepped past it), leaving the student behind a white screen
  (`/home/claude/run/live/shots/fish2/ph_5.png`). Fixed (docs/COOP.md, "Phantasm in a session"): the guest's and the
  host's casts both meet the fortune-teller and end free to move (`shots/phantasm/p2g_1_teller.png`,
  `p2h_1_teller.png`), the other console told; a guest's cast changes only its own record.
- **The bank's safe deposit (2026-10-26, by pad)**: the host's George shows the normal menu and its safe deposit
  works while connected (a wardrobe dragged in with the stylus, OK, "Thank you."; `shots/bank/h_dragged2.png`); the
  guest's Use safe deposit answers "I'm sorry, but you can't use that service while visiting another town."
  (`shots/bank/g_refused.png`): its safe deposit is in its own town, not loaded during the visit. **Built
  2026-10-27** (2c8d88b, docs/COOP.md "The safe deposit"): the guest's own safe deposit, taken from its own save,
  served in the screen in the host's counts' place and saved with its student at the visit's end. By pad and touch
  (AAAAAA host, NORA guest, `/home/claude/run/live/shots/sd/`): the guest deposited its wardrobe (`g06_dragged.png`),
  withdrew it in a later visit (`r7_12_withdrawn.png`) and deposited it again, ended the visit (its own Disconnect,
  and in another run the host's), and loaded alone found it in its own safe deposit (`s1_08_k1.png`,
  `s2_03_found.png`); the host's bag, safe deposit and save were unchanged throughout. With the host's game killed
  after a deposit, the guest's save kept the wardrobe in the bag (no loss, no copy).
- **A host's game killed mid-visit (same-PC link)**: the guest stopped at that frame, waiting for the host's frames
  (6 % CPU, no "communication error" in 10 minutes); closing it was the only way out. Over the internet mode the stand-ins
  see the death and leave (docs/COOP_INTERNET.md). Seen 2026-10-27 (`/home/claude/run/sd/r5_guest_drop.log`). **Fixed
  the same day** (see "Still open"): melonDS asked for the dead host's frames on every timer tick with a 25 ms wait,
  and the link never learnt the host was gone.
- **A silence of 5 s** (a debugger pause here) makes a player's stand-in leave the session ("the far player left");
  the leave met "Unable to disconnect." and the stand-in stayed, so the session survived; a real 5 s network outage
  would end the far player's visit on that PC.
- **The guest's clock** runs a few minutes off the host's in unpaced runs (docs/COOP_INTERNET.md, Open).
- **A game closed right after a visit's error** ("Unable to receive data", then turned off within seconds) loads
  into the warden's punishment next time: the game's own rule (docs/COOP.md, "Turning off without saving").

- **The top screen place by place (2026-10-27, real pad input and warps, a connected pair of day1done.sav and
  second_day1.sav, Connect now, `--clock 2000-05-21T15:30`).** The black top screens upstairs in the dorm right after
  connecting were the game's own top camera (black alone too); now the corridor and the Gateway. Every other place
  (dorm lobby and room, school hall, classroom, auditorium, Sol's office, town, Harrows, café, Mon Paris, books, post
  office, bank, Lovers Point, beach, forest, area 63, the other world's road and tower) shows on both consoles what
  single player shows; rooms without a top view are black with the signal icon, as alone. Table and shots:
  docs/COOP.md, "The top screen place by place". Seen beside it: a classroom visit on this date starts the homeroom
  farewell ("I'm afraid I have some slightly sad news"), whose talk then holds later warps.

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
- **The bottom screen's panels with 3D on both screens (2026-10-26, real pad input, a connected pair of day1done.sav,
  Connect now, `--clock 2000-05-02T10:00` / `2000-07-18T14:00`; shots `/home/claude/run/live/shots/d3p/`).** Each
  panel against single player (a solo copy of the same save) and the DS's session layout:

  | Panel | 3D on both screens, before | after the fix |
  |---|---|---|
  | The bag (touch the bag icon), its item name | name over the top screen, bar empty (`bag1.png`) | right, host and guest (`fix_bag.png`, `fin_bag_guest.png`) |
  | The bag's money (Ritch) menu | right | right (`fin_bag_guest.png` run) |
  | A talk menu's title (Principal Sol, "Introduction") | title over the top screen (`g_sol_menu.png`) | right (`fix_sol.png`) |
  | A catch's FISH panel | panel top (as alone), bottom sky (`sess_caught.png`) | as alone (`fix_caught_pair.png`, `solo_caught.png`) |
  | Text boxes (Piquard at Harrows, Sol, a locked door) | right | right (`harrows1.png`, `fix_sol.png`, `in1.png`) |
  | The Start menu: Wizard ID / Info, Magic, Incantation, Town Map, Telegrams (letters), Friends | right (the menu's own screens) | right (`btn_grid.png`, `tab_142.png`, `tab_grid.png`) |
  | A shop (Harrows, the café): the clerk's talk | right (Harrows keeps its top view; the café is 3D on the bottom only) | right (`harrows2.png`, `g_cafe1.png`) |

  Not reached by the driver: a shop's buying screen (Harrows' clerk only talks; buying is from the displays) and a
  save prompt (only the session's own "Saving." box). Seen once, reproduced 2026-10-27 (12, insects; a test tool's path): a guest warped (`MQ_WARP_FILE`
  `e 240`) onto the school's arrival point where the host stood with its bag open, after the host's catch, stayed
  without its student or icons and could not move, and a second warp left it loading (black); the same warp in a
  fresh pair, with and without the host's bag open there, was fine.


## Soak: one long session, features combined (2026-10-27)

What two players do in one sitting, driven by real input and scripted with small random variations, over the internet
mode at the 200 ms test link (`NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1` on both copies: 202-231 ms round trip in the
window's title, ~1 % of datagrams dropped), host `day1done.sav` (AAAAAA), guest `second_day1.sav` (NORA), with both
consoles' state compared after every step and every two in-game minutes (harness and what is compared:
docs/TESTING.md, "The co-op soak"; driver `/home/claude/run/live/soak.py`, logs `/home/claude/run/soak/logs/`, shots
`/home/claude/run/live/shots/soak/`, `shots/cast/soak1fr_*`, `shots/inet/gift_soak1_*`).

**What ran.** Day 1, Tuesday 18 July 2000 from 14:00 to 15:07 in the game (the copies unpaced: ~240,000 frames, about
50 minutes of play): out of the dorm and through town together; apart in two districts (the bookshop's side and the
school's front) and back; a chat each way; a gift each way; both into the bookshop and out; both fishing by the
river; the net after an insect; Friendship at the ruins (the guest transforms and says the words, the host holds the
emerald: "Your Friendship incantation reached NORA!" / "AAAAAA cast the Friendship incantation with you!", +2 on
both `.coop`); homeroom (Eric's farewell) and a lesson; the walk-in mystery (area 10, scene 12104, the other asked and
joined with the step beside); the host's Disconnect (both "Save complete."). Day 2 from those saves at 02:55 on the
19th: connected, the day turned in the session at 3:00 (summer: the town's periods table starts the day at half hour
6), the host's Disconnect for the new day, both saved and connected again 30 s later, each console's copy of the
other student where the other stood; walking at night to the school's front (its door shut at night, for both, as
alone); the Gateway upstairs in the dorm to the other world (the host's menu, the guest asked "Go with AAAAAA?", Yes),
both to the tower, the old man's scene joined by the other ("near the other student: joining scene 10203 at once"),
the principal's return scene (10204) and both back in the dorm by the Gateway; the host at Lovers Point (the guest's
student was held in a talk menu by the driver there). Day 2 ran from 02:55 to 04:19 (~290,000 frames, about 40
minutes).
Earlier passes of day 1 (the first ~40 minutes, two restarts while the driver was fixed) are in the same logs. 171
state comparisons (`soak1.log`, `soak2.log`); the frame rate 62-141 frames a second per copy (load 4-10, another agent's pair running); no
fatal line, no process lost, no stall over 3 s except one of 4.1 s on the guest while a build ran (load 9), no frame
rate drop.

**Same on both consoles throughout**: the town block outside the bytes below (a classmate's needs, fixed since), the date, the sky kind, the weather
bytes, the host's variables once no scene ran, the pair records (`.coop`; the `@counter` lines are each console's own
gift bookkeeping), each student's place against the other console's copy of it whenever both stood still in one area,
the bags after each gift, the new day's turn.

### Found and fixed

- **The communications room stayed black when the other student's talk arrived as this one went into it** (day 2,
  03:55). The host pressed A at the Gateway upstairs in the dorm while connected (the game's own session menu, "Where
  would you like to go?"), and in the same seconds the guest pressed A facing the host (our talk: "slot 1 came over to
  talk" on the host); the host then picked the first option, the communications room. The host's console loaded area
  16 and started our talked-to event (90003) before the room's own (27101): the room's "Disconnect?" never showed,
  both screens stayed black but for the bag icon for over five minutes, the student could be moved in the empty area,
  and only a warp brought it back (`shots/soak/d2_x_host_in_room.png`, `_room2.png`, `_back.png`; log
  `/home/claude/run/soak/logs/soh_day2a.log`). Picking the room without a talk going on shows the game's "Disconnect?"
  as it should (`shots/soak/room_2_after.png`). Fixed in fe16f5e (coop_talk.cpp): the talked-to event remembers the
  area the student stood in and is dropped ("... but this student is in area 16 now (was 14): not shown") if the
  student is elsewhere when it would start. The same input after the fix (`/home/claude/run/soak/room_talk.py`):
  "Disconnect?" on the host, the guest's own talk box ("Oh, hi, NORA!") goes on (`shots/soak/roomtalk_2.png`). QUICK
  PASS.

- **g1040 after a player talk stayed 1 on the guest and 0 on the host** (from the first chat to the end of the visit;
  seen in the soak's checks after the shop step, "DIVERGENCE ... ('g1040', 0, 1)"). Every player talk writes g1040 = 1
  on the guest (sent to the host as a request); the host's game sets it back to 0 soon after. When that happened
  before the host's next comparison (once a second), the host's block was back at the value it had last sent, so
  nothing was sent, and the guest kept its 1 (its request answered). The logs show the ping-pong (`request n -> host:
  1 bytes (+300:1)`, `town from host: +300=0`) ending on the guest's value. Fixed in 0846e7c (coop_sync.cpp): the bytes
  a guest's request wrote are always sent at the next comparison, whatever they are by then. Verified in the soak's
  next pass: after both chats and both gifts "town from host: +300=0" each time and no g1040 difference; QUICK PASS.

- **Over the internet mode the host's own Disconnect (Select, Yes) sent the guest back to search for a town** (day 1's
  `leave` step). The guest saw the game's "The person you were playing with has disconnected.", "Disconnecting.",
  "Saving.", "Save complete.", and then `coop day: the host is no longer in this visit: ... joining again`
  (coop_session.cpp `host_gone`) started Connect now as a guest: the communications room, "Searching for a town to
  visit...", "Unable to find a town to visit.", "Would you like to search for ...", again and again (the room's 30
  searches; not "Connect automatically" itself, which runs once a run). Cause: over the internet the real host leaves
  its session before the stand-in host on the guest's PC has gone through its own Disconnect, so the guest's
  `coop_net` sees "the far host out of a session for 3 s" (the split meant for a new day that a guest's open text box
  held up) and `host_gone` always went home and joined again; nothing told a host that chose to stop apart from the
  new day. Reproduced at 200 ms (`/home/claude/run/sl/disc_test.py` after `soakpair.sh`, the current binary:
  `/home/claude/run/sl/repro_1_guest.png`, "Searching for a town to visit..."). Fixed in e73a460: the guest joins
  again only if the host's "the visit ends for the new day" (side channel 41, kDayAsk) was heard in that session;
  otherwise "closing what is open, then home (the host's own Disconnect: not joining again)". The same test after
  the fix: both "Save complete.", the guest home by the Gateway, free (the bag icon), no search in the next 60 s of
  its frames (`/home/claude/run/sl/fix1_1_guest.png`, `fix1_1_host.png`). The new day still connects both again (the
  host's kDayAsk is heard first; day 2's `newday` step from the day-1 saves at 02:55 with this build: "the host ends the visit for the new day", "the visit ended", "connecting again as guest", connected, in step, log `/home/claude/run/soak/logs/soakslnd.log`). QUICK PASS.
- **A joined scene that wrote the town's dated list wrote it twice on the guest**: after the other world's return
  scene (event 10204, the principal taking the key back, played on both consoles) the list of 0xE4-byte entries at
  town block +0x6F0C (newest first; event op 0x73 adds one) got its new entry at the front on the host, which the
  town sync sent; then the guest's own 10204 ended and its game put the same entry at the front again: on the guest
  [new, new, old], on the host [new, old], for the rest of the visit (a change the guest's game made by itself is kept
  there, and the host never changed those bytes again). Fixed in 564d2bd (coop_sync.cpp): the list (+0x6F0C up to
  the waiting list at +0x7AA8) is the host's; a guest's own changes to it are not sent, and once no event runs on the
  guest the host's list is written back ("coop sync: the town's dated list as the host has it again (9 bytes this
  console's scene wrote): +6ff0=149 +6ff2=172 +70d4=0 +70d5=0 +70d6=255 +70d7=255 ..."). Verified with the soak's
  `gateway` step on a fresh internet pair at 200 ms (`soak.py sl 7 gateway`, log
  `/home/claude/run/soak/logs/soaksl.log`, shots `/home/claude/run/live/shots/soak/dsl_12*.png`, `dsl_13_gate_back.png`):
  both consoles' +0x6F0C..+0x7AA8 identical afterwards ([new, old, free, ...] on both: `ec1b4800 00061102`,
  `951bac00 00061102`, `0000ffff ...`), the town block in step at every check. (One periodic check during the
  guest's own 10204 saw g1001/g1009/g1054/g1070 differ: the host's alone, kept by the guest while its scene runs; in
  step once it had ended.) A scene played only on the guest that adds an entry there now keeps it only
  while it runs (the guest's copy of the host's town is not saved anyway). QUICK PASS.
- **A classmate's +0x48..+0x4F and +0x9F differed between the consoles for minutes** (classmate 0, 5, 6 or 7, two
  to six bytes, e.g. classmate5+0x9F host 16 guest 0 for 13 minutes, classmate0+0x48.. 55/57, 26/24, 31/33, 21/22;
  the guest's "coop sync: 4 bytes this console changed by itself, kept here: +64b0=55 ..."). **What they are:** each
  classmate's eight needs (docs/COOP.md, "The classmates' needs"): set at a new game from the classmate's parameters,
  moved by one or two units when the classmate starts or keeps up a pastime (ov001 0x020EBD4C -> 0x020EBE68 in the
  director's round, 0x020EBFC0 at a job's start, both through main 0x0202F388 -> 0x0202F144, which also rewrites
  +0x9F), read by the classmates' talk rules (ov001 0x020F60A0: which lines and moves a classmate picks) and a field's
  set-up of its people (main 0x02043408). Found with a range write watch (new: `MQ_WATCH_LEN`, `MQ_WATCH_STRIDE`,
  `MQ_WATCH_KEEP`, docs/TESTING.md) on both copies of an internet pair at 200 ms through the soak's `town`, `apart`
  and `shop` steps (`/home/claude/run/cm/wsum.py` prints the writes): on the host `0202f389 <- 020ebee3` (the
  pastime's table) every ~3,600 frames for classmates 0, 2, 5 (e.g. classmate 0: 56 24 . 32 -> 55 26 . 31 -> 54 28 .
  30, classmate 5: 72 40 -> 71 42 -> 70 44 ...); on the guest the same writes only for a classmate the guest decided
  (NPC 72 while the guest stood nearest to it: frames 54539, 60908, 64494, 68245 of the guest), none for the others
  (their copies start no jobs). The soak's "the same values again at each field load" was not seen again (no write of
  these bytes at any of the guest's loads in these runs); a change of that kind falls under the same rule below.
  **Why they differed:** only the console that decides a classmate starts its pastimes
  (coop_npc.cpp), so a classmate the guest decided had its needs moved on the guest only; those were "kept here" and
  never sent, while the host's game left that classmate alone, so the two stayed apart until the host decided it again
  and moved it from its own old values. A difference matters little on screen (the classmate's place, walk and pose
  are the decider's on both screens: `shots/cm/needs_pair3.png`, NPC 72 at the school's front on both, standing), but
  a talk with that classmate on each console read different needs. **Rule chosen:** the console that decides a
  classmate moves its needs for the town: a guest's change is sent to the host as a request when the guest decides
  the classmate (or a scene runs on the guest: a talk, a gift), the host's values go back at once when another console
  decides it, and a change made just after a field's load, before the host's word about where it stands, waits (300
  frames at most, then the host's values). Not a give-back after a delay (the reverted patch fought the guest's own
  writes); not excluded from the comparison (the values are read by the game). Fixed in 6033740 (coop_sync.cpp
  `classmate_needs`, coop_npc.cpp `mq_npc_decides`; the host's people message now also goes without people, so a guest
  learns the host stands elsewhere). Verified on an internet pair at 200 ms (`/home/claude/run/cm/cmpair.sh`, the
  soak driver as `/home/claude/run/live/soakcm.py`, whose comparison now counts a needs difference as a divergence,
  `SOAK_NEEDS_NOTED=1` for the old way): the guest's "classmates' needs this console's game moved: sent to the host
  (this console decides them ...): 0 (id 72)" -> "request 15 -> host: 4 bytes (+64b0:2 +64b3:1 +64b5:1)" -> on the
  host "request 15 from slot 1: 4 bytes written", four times, the 13 records the same on both at every sample after
  (`/home/claude/run/cm/cmwatch.py`, a sample every ~30 s); a poke of classmate 1's needs on the guest (decided by the
  host) went back at once ("the host's values back (another console decides them): 1 (id 6)"), one of classmate 0's
  (decided by the guest) went to the host. The soak's `town, apart, chat, gift, shop, fish, insects` with the fix
  (`/home/claude/run/cm/soak1.log`, 31 minutes, 29 comparisons): no divergence, the needs included (one transient
  classmate 6 difference re-read the same 4 s later: the host's change on its way); then 15 minutes of samples at the
  school's front and the plaza, the needs the same at every one. QUICK PASS.

### Found, not fixed (with what reproduces them)

- **The school's door did nothing for the guest once** (day 1, 14:57, after Friendship and changing back): the guest
  warped to the school's arrival point (`e 240`), walked to (44, 24.6), Up three times, A: no event (its log shows only
  the arrival's 25342 before), the guest stayed outside; the host went in from the same place two minutes later. The
  same as the open item above ("The school's door once refused both players"); the driver's place in front of the door
  was not checked closely, so it may be the stand point. At night (3:2x) the door opens for neither (no text): the
  school is shut, as alone.
- **The other student's copy reads "not here" (+0x5A0 = 0) on some frames while that student walks** (6 of 40
  samples of the host's copy of the walking guest, all while walking; never while standing; `soak_here.py`). Coop code
  that tests +0x5A0 once (coop_magic's join test, the insects' nearest student) could miss a walking partner.
- **The two consoles' clocks drift apart in unpaced runs** (3-4 minutes after the host had run ~11,000 frames more than
  the guest): each console's clock is its own emulated time (the known item in docs/COOP_INTERNET.md, "Open"); at the
  DS's 60 frames a second both run alike.
- **Soak, release build d4bda9f (2026-10-28): a persistent town-block difference from the first walk on day 1**
  (`soak.py 1` after `soakpair.sh 2000-07-18T14:00:00`, binary of /home/claude/verify at d4bda9f, 200 ms link): first
  check "same", then from 05:54 (periodic@23700, during `apart`, after `town`) every comparison to the end logs the
  same set: town block +0x2ba (host 14, guest 15) and classmate0 +0x18..+0x1f (host 255,255,255,255,16,0,255,255;
  guest 251,52,129,30,192,17,65,0), 37 of 42 checks; classmate3 +0x48/+0x49/+0x4b (needs, 60/98/60 against
  61/96/61) once at 161388. Not seen in the 2026-10-27 run. **Fixed the same day** (docs/COOP.md, "The classmates'
  happenings and the talk's turn"): classmate +0x18..+0x47 are its six recent happenings (8 bytes each; the guest's
  was stamped 14:04) and +0x2BA the town's four-bit turn through them for talks, both written by the guest's game
  for a classmate it decided and kept only there; now under the needs' rule. Three more runs of the old build at
  14:00 (with a range write watch on both bytes, `MQ_WATCH=020AF2DA MQ_WATCH_LEN=0x61CE MQ_WATCH_STRIDE=0x61C6
  MQ_WATCH_KEEP=0-8`) did not show it again (it needs the guest's director to make a happening, which depends on
  where the classmates walk); with the fix, a poke of both on the guest (classmate decided by the host) went back
  to the host's values at once, `town,apart` (9 checks) and the whole day 1 (`soak.py 1 1108`, 40 checks, 08:11-08:43,
  binary of 6777f45 + the fix) logged no divergence at all; the guest's log had five `happenings ... sent to the
  host` (classmate 0, id 72, which the guest decided; the host wrote all five requests). Day 2 (KEEP=1,
  02:55 on the 19th, 50 checks): no town-block difference; one `pos ... not here` at the Gateway's other world
  (the +0x5A0 item above) and the school door shut at night for both (as above). No fatal, stall or frame-rate drop
  in either day; frame rate 47-100 a second per copy (load ~5).

Seen on the way, not faults: a second chat on the same day answers "You've already had a good long chat" (the game's
once a day); A pressed by a door with the other student in front of it starts our talk instead (the known item above;
the driver now steps the other aside); the driver's emerald poke wrote the bag slot where the guest kept the gift.
Not reached: a fish bite (both casts came back without one) and an insect catch (the nearest insects were across the
river or flew off).

## Insects over the internet at 200 ms, real input (2026-10-27)

Internet mode at the 200 ms test link (`NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1` on both copies, 210-217 ms round trip,
~1 % of datagrams dropped), the release build of f99d57b, host `day1done.sav` (AAAAAA), guest `second_day1.sav` (NORA),
`--clock 2000-07-18T14:20`, Connect now, both warped to the bookshop's front (`e 243`), `MQ_COOP_INS_LOG=1
MQ_NET_LOG=1`. Students moved only by the pad (short D-pad presses, 6 frames, near the insect), the net taken with R,
swung with X (driver `/home/claude/run/ins200/catch.py`, logs `/home/claude/run/ins200/{host,guest}_log.txt`, shots
`/home/claude/run/live/shots/ins200/`). Both consoles listed the same insects in the same slots, kinds and places
(RAM, before and after each try, to 1/100 of a unit while they move).

- **The guest's catch, the host near: works.** A cicada (kind 11, slot 11, on a low tree by the bookshop), the host
  standing 3.6 units from it: guest "our net caught insect 11 (gen 5): asked the host" -> host "slot 1 asks for insect
  11 (gen 5): yes, it is theirs (removed here)" -> guest "insect 11 is ours (the host says yes): caught", "You caught a
  Large Brown Cicada!" with the INSECTS panel on the guest (`g5_after.png`, left the guest, right the host's view of
  the guest's swing, the tree empty). The guest's bag gained the insect (item 0xA849: 2110 + 11 with the size bits),
  the host's bag unchanged; the insect gone from the host's slots and inactive on the guest's.
- **A butterfly the guest swung at was given to nobody (open, not reproduced).** A Small Cabbage White (kind 0, slot
  0), the guest 1.1 units from its place (+0xFC), the host about 6-7 units away: the ask crossed and was answered
  ("slot 1 asks for insect 0 (gen 1): no"; the guest "insect 0 is not ours (the host says no)", the swing ended as a
  miss, then "insect 0 (gen 1) gone at the host"). On the host the insect object stayed in its slot but inactive
  (+0x13C 0, watching the guest's student object), no student holding it (+0x5DC: 65 on the host's own, 255 on its copy of the guest; neither 0), no bag
  changed: both consoles agree it is gone, and neither got it. The host's swing-copy of the guest arrived in the same
  frame as the ask (move type 11 state 04, host frame 184853). Which of the host's tests said no (active, catchable,
  the slot's number) is not logged; likely the host's copy had already flown off as the guest's student reached it
  (100 ms late on the host), the game's own flight, but an insect turned inactive without a catch is not what a
  flight does to the cicadas (they stay active, high up). To look into: log the failed test in `host_asks`
  (coop_insects.cpp) and swing at butterflies with the host near.
- **The host's catch, the guest near: not reached in the time.** The guest stood 3.6-4.5 units from each; a cicada
  high on the cliff by the bookshop (slot 17, y 4) cannot be reached from below; a running insect (kind 14) ran off
  west along the ground as the host walked up, on both consoles alike (-5.92 / -5.92); a kind 19 (y 1.5) in a tree
  missed 11 swings from 1.0 unit (not visible: not a net insect from the ground); the butterfly (slot 2) fluttered
  round, one swing at 1.3 units missed, no ask (the host's own catches need none). The host's own catch with the
  other near was verified earlier on the LAN (above, 12). A second pair from the same clock rolled the same insects
(same slots, kinds and places): the cicada the guest had caught (slot 11) ran off west as the host walked up to 2.6
units (on both consoles alike); another (slot 16, the guest 3.9 units off) let the host reach 0.9 units, its swings
(facing it, 0.9 down to 0.1 units) missed and it turned uncatchable (+0x189 0) on both consoles (`h7_after.png`).
Seen: the insect watches the nearest student and changes over only when another is nearer by a unit, so it watched
the standing partner while the catcher closed in from 3-4 units; the catcher who is also the watched student is
sensed all the way (the game's own flight, as alone). No fatal line, no stall; both pairs stopped cleanly.

## Insects at 200 ms: the butterfly lost to both, fixed (2026-10-27)

The same link and setup as above (internet mode, `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1` on both, host `day1done.sav`
AAAAAA, guest `second_day1.sav` NORA, `--clock 2000-07-18T14:20`, both warped to the bookshop's front `e 243`,
`MQ_COOP_INS_LOG=1`; the pair from `/home/claude/run/live/inetpair.sh` with `MQ_BIN` = the build, drivers
`/home/claude/run/ins2/{pswing,place,held}.py`, shots `/home/claude/run/live/shots/ins2/`). The same insects rolled
again (slot 0 the Small Cabbage White by the tree house at (19.41, 65.50)). To get the net to a flying butterfly in
the time, the catcher's student was put 0.5 units east of it (a poke of its place: a jump, the other console shows a
dash), faced it with one press of Left and swung with X (real input); each try a few frames late, as by hand.

**Cause** [S, then reproduced]: the net's state function chooses a swing at something in front (a wall or tree,
main 0x020297B4 -> step 10) or, in the open, a sweep through the air (step 30). The swing in front runs the catch test
only for the local student (+0xAA, main 0x0202A00C), the sweep for *any* student (main 0x0202A0AE, all kinds). On the
host the copy of the guest's student sweeps too, ~100 ms late, and its catch test caught the butterfly on the host:
+0x13C 0, the copy's +0x5DC the slot (later -1 again), and the copy's branch (main 0x0202A134) never takes it. The
guest's own catch (its console's sweep) asked; the host found the insect inactive and said no; the host's state left
it out and the guest deleted its copy: nobody had it. Cicadas on trees were caught (a swing at the tree: step 10, no
test for the copy). Reproduced with the old behaviour on the host (`MQ_COOP_INS_REMOTE_GAME=1`): host "insect 0 here:
active 0, catchable 1, near 0, watching 02291b48 (the guest's copy) ... slot 1 asks for insect 0 (gen 1): no: inactive
here (caught)"; guest "insect 0 is not ours (the host says no)", "insect 0 (gen 1) gone at the host"; RAM after: on
the host slot 0 inactive at (19.70, 63.40), no student's +0x5DC 0, no bag changed; on the guest slot 0 empty
(`old1_after.png`). The original report, exactly.

**Fix** (coop_insects.cpp): a partner's copy's catch test is run and undone (`remote_catch`), on both consoles; and
the host refuses a guest's catch only when the insect is no longer free there (caught by the host's student, given to
a guest who asked first, another insect in the slot), not when its copy flew off or turned uncatchable within the
round trip: in single player a net that connects catches, and the guest's net hit the insect where the host's state
had it. A refusal no longer makes the guest's copy uncatchable (it mirrors the host's flag). `host_asks` logs the
insect's state and the refusing test.

- **The guest's catch, the host far (~15 units, at the bookshop's door)**: the host's copy's sweeps hit the butterfly 2-6 times per run
  ("the partner's copy's sweep hit insect 0 here: undone (its console decides)"), then the guest's net: "our net
  caught insect 0 (gen 1): asked the host" -> host "insect 0 here: active 1, catchable 1, near 1, watching 02291b48,
  gen 1, at (19.33, 0.92, 65.81)", "slot 1 asks for insect 0 (gen 1): yes, it is theirs (removed here)" -> guest
  "insect 0 is ours (the host says yes): caught at the swing's end", "You caught a Small Cabbage White!" with the
  INSECTS panel (`p3_after.png`, `g7_after.png`, `g8_after.png`: left the guest). Item 26686 (2110 + kind 0, size
  bits) in the guest's bag; slot 0 empty on the host, inactive in the guest's hand. Three pairs, three catches.
- **A second bug found on the way: the guest's message left open**: the guest that had caught the butterfly left
  "You caught ...!" open; ~20000 frames later the guest ended in `fatal: no recompiled function at e92d4008 (lr
  0202a6ef)`: the net draws the held insect (student +0x5A4, main 0x0202A6EC) every frame of the catch's steps 60-90,
  while +0x5DC is already -1, so once kMineFrames had passed the copy was deleted as gone at the host and the draw
  jumped through the freed object (r4 = the held object, its table garbage). Fixed (a1daf90): an object the local
  student holds while its net's steps run is not deleted. Verified: the message left open 2200 frames (held object
  kept, step 90), then closed: the student free, the guest's slot 0 deleted then (as on the host), no fatal
  (`g8_closed.png`).
- **The host's catch, the guest near**: the guest put 2.5 units south of butterfly slot 3 (34.46, 40.50, below a
  rock ledge by the waterfall), then the host (from the bookshop, 33 units) put beside it and swinging: the guest's copy of the host's sweeps
  hit the guest's copy 5 times ("undone"; before the fix each of them hid the butterfly on the guest for good), the
  third swing caught: "You caught a Small Cabbage White!" on the host, the guest's copy "insect 3 (gen 4) gone at the
  host" (`h1_after.png`: left the host catching, right the guest's view, the two students side by side).
- **Does the partner scare insects more than a student of one's own?** [S + RAM] The class think's sensor (ov001
  0x020DCFEC) reads only the distance from the insect's place to the watched student's +0x2C and the height
  (+0x30); no speed or state. The host's copy of the guest has the guest's place and height (RAM, at the same frame:
  (19.91, 0.00, 66.10) for the guest's own student and the host's copy of it). So a partner scares an insect as a
  local student at its place would, ~100 ms late at 200 ms. Not changed; no single-player run was needed for this.
- `quick.sh`: QUICK PASS (bf19704's code and a1daf90's). No fatal line on either console in the fixed runs.

## The partner's copy's actions at 200 ms: the whole class, audited (2026-10-27)

The butterfly above was one case of a class: the game plays the partner's student copy through the same action code
as the local student, on this console. Every action was checked (docs/COOP.md, "The actions of the partner's copy",
has the table): statically (the copy's act, main 0x020325E0, and every send, store, bag and field write reachable
from what it runs, with or without the local-student test +0xAA) and in a connected pair, internet mode at the 200 ms
test link (`NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1` on both, 208-228 ms round trip, ~1 % dropped), host `day1done.sav`
(AAAAAA), guest `second_day1.sav` (NORA), `--clock 2000-07-18T14:20`, Connect now (the pair from
`/home/claude/run/live/inetpair.sh`, `MQ_BIN` = the build, harness and logs `/home/claude/run/acts/`), with
`MQ_ACT_AUDIT` (main RAM before and after each act of the copy) and later the guard's log `MQ_COOP_ACTS_LOG`. The
students moved by the pad (a few placed beside a target by a poke of their place, as before).

**Done by real input, one console acting, the other watching** (the copy's action on the watcher in brackets):
standing and walking (0, 14); R through the four hand tools 660-663 and X with each (660 the shovel: no action on the
copy, the hole is a ground object; 661 the rod: 11; 663: 4 step 2; 662 the net: 4 step 30), on both consoles; the
rod cast and reeled at the river by the bookshop (11, "The fish got away" and "You caught a Colored Carp!"); A at the
bookshop's door and in (7); the wizard form by the D-pad circle and back, both ways (30); a talk with the other
student ("Oh, hi" ...); a net catch by the host with the guest near; mushrooms taken by A (8, the cell cleared on
both consoles, the item in the taker's bag). In none of them did a copy's act change the town block, the watcher's
record or bag (the audit), and no packet left the watcher on the copy's behalf; the write watch on the town's store
byte 74 (`MQ_WATCH=020AF322`) saw no write. Shots: `/home/claude/run/acts/` (`u66*.png`, `w66*.png` the tools,
`door4.png` the guest in the bookshop and the host outside, `wiz1.png`, `f1_cast.png`, `pk3_*.png`).

**Found: a full bag's choice cut short by the partner's pick-up** (fixed). The host's bag poked full (15 mushrooms),
the host catches butterfly 0 by pad (`ins2/pswing.py`), closes "You caught ..." to "Select an item to remove" (the
local action 16 step 25, the field's catch view +0x135FE = 1); meanwhile the guest takes a mushroom by A (47 units
away, cell (66, 70)). Harness `/home/claude/run/acts/lost2.sh <tag> <host env> 66 70 <place|discard>`.
- Guard off (`MQ_COOP_ACTS_OFF=1` on the host; `lostB`, `lostD`, `lostF`): the host's catch view 1 -> 0 at the
  guest's take, each time (the guest's copy runs action 8 on the host; its step 0 clears it, main 0x0202358A). In
  `lostD` the host's choice then ended by itself with no input: action 0, the insect in bag slot 0 and that slot's
  mushroom on the ground beside the host (cell (18, 66) = 266). In `lostC` (the second insect of a run, the same
  steps) the host crashed instead: `fatal: no recompiled function at 00000000 (lr 02031c7f)`, in the copy's 0x1D
  packet case (main 0x02031B50, a call through the copy's model +0x1E8); not seen again (open, docs/COOP.md).
- Guard on (`lostE`): "coop acts: the partner's copy (action 8 step 2) wrote field (catch view)+0x135FE (1 -> 0)
  here: undone"; the host still at "Select an item to remove from your item window." with the catch view 1, until
  its own choice (`lostE_end.png`, left the host).
- Verified on the game's path too: the host's copy put in action 8 (a poke) with the host's catch view at 1: undone,
  the view stayed 1.

**Fix** (e124a1b, game/src/coop_acts.cpp): the copy's act is wrapped; what it changed of the town block (but its
time), the local student's record, the field's catch view and message request is put back after it (a guest's
store writes inside it are not sent to the host as its own). Not reached by pad in the time: an item put down from
the bag and held up (26, 22), seats (19, 22), the copy's "bag full" message in areas 22-25. QUICK PASS.

## October hotpot cook-off by real input over the internet (2026-10-28, report only, no game code changed)

Pair over the internet mode with `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1` (`inetpair.sh 2000-10-03T15:00:00 hoth hotg`,
port 49871, binary `mqv` of /home/claude/verify, host `day1done.sav` AAAAAA, guest `second_day1.sav` NORA), `var 411=0`
on both, host into the town (`e 239`), guest in the dorm, then the host warped to the forest (`66 5 15`). Shots
`/home/claude/run/live/shots/hot_*.png` (`hot_p*` are pairs, host left), logs `/home/claude/run/live/hoth|hotg/log.txt`.
- Oakley stands at the cauldron in the forest (map 66, about (6.9, 5)); the warp itself runs the set-up (50211).
  Standing at (6.42, 5.7) facing up, A: his first talk ("Greetings, AAAAAA. I am Mr. Oakley...") and the briefing,
  scene 50212, **started by the host's talk** (request kind 0 allowed). Works.
- The guest (in the town, far away) was asked "AAAAAA is at the mushroom hotpot cookoff!" / "Go along with
  AAAAAA?" (`hot_p2`, `hot_p3`); Yes: into the forest beside the host (7.41, 5.69, `hot_p4`). Works. The host's briefing
  had already ended by then (it is short: "the other student's scene 50212 ended"), so the guest joined a finished scene
  and was not in the briefing itself; it then talked to Oakley itself and got its own briefing to the end (`hot_p8`,
  `hot_p9`), the host seeing "In event..." over the pair. Both scenes ended cleanly (`scene 50212 ended here` on both).
- Gathering: the forest has no mushrooms on the ground, they are the town's grid (items 256-279). The host walked out
  (south, to the town at (10, 39)), took a Truffle (257, cell (14, 30)) and a 273 (cell (16, 30)) by A (real input; the
  bag in RAM), and walked back in by the forest's entrance; the first walk Up from (10, 39) enters the forest again, so
  routes to the mushrooms go Right first. Other classmates were not seen in the forest in the pair (a solo control on
  the same save and clock showed a penguin and a rabbit student with mushrooms on the ground by the cauldron after the
  briefing: `hot_s2.png`, `hot_s3.png`; not seen in the pair, where `coop npc: the people here live as in single player`
  is logged; not followed up, possibly only the timing).
- "I brought something back" opens the bag on the touch screen with "Then, can you give me the mushrooms you collected?",
  and no tap, A, drag or button I tried hands a mushroom over (tap moves the cursor and shows the name, A hides it,
  a drag out of the bag cancels: "Well, if there's anything else I can do for you"). The same in the solo control
  (`hot_s4`, `hot_s5`), so it is the game's, not co-op's, but **the cook-off's give-mushrooms step was not completed
  by input**; B always backs out cleanly. Open: how a player gives them (not found).
- "Let's eat" (the menus are touch: tap the line) and Yes: the feast, scene 50213 / 50214 ("All right then, let's get
  everybody together", "Ho ho ho!", the results with nothing handed in: "Um... The hotpot had nowhere", "And didn't I
  tell you to bring...", "We'll do it again next year") ran to its end on the host (`scene 50214 ended here`) while the
  guest, beside it, was asked nothing (within 12 units) and showed "In event..." over the host; then the guest ran its own
  feast the same way, the host showing a thought cloud, both ended cleanly (`hot_p33`, `hot_p38`). So both took part,
  each alone, as designed; the feast is not shared.
- No hang, no fatal, no divergence message in either log; frames advanced on both to the end (150258 / 151500); the town
  block (0x993C bytes) of the two consoles differed in 13 bytes: the checksum (+0, +1), the clock minute and the time
  offset (+0x2CD, +0x2CE, +0x2D4-7), as known, and +0x3D5-0x3D7 and +0x6948, +0x694C, not looked at (the festival's
  state; probably the host's own set-up of the week).
- Not done: the guest's own mushroom pick-up in the town, the classmates' part, the present after the feast (nothing was
  handed in), a re-run of the festival the next day.

## The cookoff's hand-over and the two town-block differences (2026-10-28, docs only, no game code changed)

Follow-up of the entry above. Binary `/home/claude/run/hot/mqv` (the same as that run). Solo: `day1done.sav`
(AAAAAA), `--clock 2000-10-03T15:00`; pair: same-PC link, Connect now, host `day1done.sav` (AAAAAA), guest
`saves2/second_day1.sav` (NORA), link `hp2pair`. Shots `/home/claude/run/live/shots/hp2_*.png`.
- **How a mushroom is handed over** (not a recomp fault; the game's bag): "I brought something back" opens the bag in
  its give mode (query 0x03, the bag's mode 1, as giving a classmate an item). Touch the mushroom once (it is
  selected: cursor and name), then press on it again and drag it out of the bag to the ground in front of the
  student (170, 105): the student holds it out and Oakley answers ("Hmm, that's what you want to put into the pot?",
  "All right then, just keep bringing me back those mushrooms!"; the Fly Agaric: "Have you lost your mind?!", back in
  the bag). What failed before and here: a drag starting on another slot than the selected one only selects it, and a
  drop onto Oakley / the cauldron above the student (about (171, 68)) leaves the item in its slot without a word
  (`hp2_d1`..`hp2_d4`); the gift tests' "a drag from the second slot only showed the name" is the same first rule.
  Solo: Truffle (257) and 273 handed in (RAM: both gone from the bag, hand empty; `hp2_tb.png`), the Fly Agaric
  refused (`hp2_g1.png`); the feast (Let's eat, Yes): "Um... The hotpot had nowhere...", "And didn't I tell you to
  bring...", "We'll do it again next year" (the lowest result: the script grades on s52 against 9; two common
  mushrooms are not enough).
- **In the pair, both hand in**: the host gave 257 and 273, the guest (which, in the dorm, had been asked "Go along?"
  for the host's briefing but answered after it had ended: "Looks like AAAAAA has already moved on", so it came by
  `e 221` + `66 5 15` and talked to Oakley itself: its own first talk and briefing) gave 257 and 259; RAM: the
  host's bag 258, 261 left, the guest's 273, 262 left, hands empty. Each then ran its own feast (Let's eat, Yes) to
  "We'll do it again next year", the other console "already beside the other student at festival scene 50214" (not
  asked); same lowest result on both. After it the host's cauldron is out and the guest's still burns until its own
  feast (each console's scene state, as designed; `hp2_q.png`). No fatal, no divergence message; the town blocks
  differed only in the checksum, the clock bytes and +0x3D5..+0x3D7.
- **+0x3D5..+0x3D7 and +0x6948 / +0x694C** (the earlier run's leftover differences): +0x3D5..+0x3D7 are store bytes
  0xFD..0xFF = g1253, g1254, g1255, the after-class scene's sequence (event 010 block 10), in `kOwnVars`: each
  student's own lesson progress (host 2 3 4, guest 0 0 1, before and after the festival). +0x6948 / +0x694C are
  classmate 7's (NPC 38) +0x64 / +0x68, friendship and affection toward player slot 0, inside the per-console
  "toward the player slots" bytes (+0x60..+0x6B): 0x21 on the host (its student, slot 0, gained a point with her) and
  0x20 on the guest (slot 0 there is the guest's own student in its own town). Both are already kept per console by
  coop_sync.cpp (never sent, never taken); noted in COOP.md. Nothing to fix.
- Not done: a big enough collection for a better result (s52 >= 9), the present after a good feast.

## Festival and holiday sweep by real input (2026-10-28, report only, no game code changed)

Pairs of `day1done.sav` (AAAAAA, host) and `second_day1.sav` (NORA, guest), same-PC link, Connect now, binary of
/home/claude/verify (e4a86af), `var 411=0` on both, the host walked or warped to the area (`e 239` / `e 243`,
`MQ_MYST_GOTO 49 ...`), the event started by A at the teacher / Santa; the last festival also over the internet mode
(`NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`). Shots `/home/claude/run/live/shots/f2/*.png`, logs `/home/claude/run/live/f2h|f2g/`.
Already done by real input before: tea party, hotpot cook-off, Mr. Graves' tournament. Not a festival: nothing is fixed
to a November date (obj.dat: only the weekly mystery 39, Nov 24-30; Dec 1-7, 11-17, 18-21 are weekly mysteries too).

| Event (clock) | Result |
|---|---|
| Rainbow snow festival, 503 (`2000-03-03T15:30`; Principal Sol stands at (60, 72) after `e 239`) | **works**, same-PC link and 200 ms / 1% loss. Host at (60.2, 70.1) facing Down, A: "Ah!", the briefing (scene 50303). The guest (dorm) is asked "AAAAAA is at the rainbow snow festival!" / "Go along with AAAAAA?" once the host's scene has started (after the first A past "Ah!"; the Yes/No takes A about 1 s after it opens, two presses). Yes: guest at the post office (62.3, 58.4), "beside the other student at festival scene 50303"; it then talks to Sol itself (its own briefing to "Please go and find some."). `scene 50303 ended here` on both, no fatal. A Yes after the host's briefing has ended: "Looks like AAAAAA has already moved on" (internet: "the offer of scene 50303 is gone (it ended)"). Not done: collecting snowflakes, the ritual, the present. |
| Christmas Eve, 110 (`2000-12-24T21:00`, night only: block 11 wants the hour <= 4 or >= 19) | **works**. `var 411=0`, `var 1000=40`, `var 1010=0` poked before the host's warp to Lovers Point (`49 7 4 0`): the set-up runs by the map load (g1001 becomes 1), Santa (actor 220, with a model) at (7, 1.5). A at (7, 3.58): "Ho ho ho!", scene 11012. The guest: "AAAAAA has found something mysterious!" / "Go and help AAAAAA?", Yes: beside the host, `scene 11012 joined here`, both run Santa's talk to its end (`ended here` on both). |
| Christmas, Dec 23 (Santa in town, 110.1) | **not reached**: see the g1000 problem below (the town load drops g1000 = 40, so the set-up never runs; queued 11001 gave no Santa at (66, 10.5) in the pair or in a solo control). |
| New Year's Eve / Jan 1, 504 (`2000-12-31T22:00`, Lovers Point) | **problem**: the set-up 50401 runs (log) but Principal Sol (field person 137 at (7, 0.6)) has **no model** in a session: nothing is drawn at the bell, the student stops at z 1.99, A says nothing, so the talk (50402) cannot be started by walking up. Solo control (same save, clock, warp `49 7 3`): Sol and the couple (actors 346, 347) stand at the bell (`solo3.png` vs `ny1.png`, `ny3.png`). Repro: pair, `--clock 2000-12-31T22:00`, `var 411=0` on both, host `MQ_MYST_GOTO` `49 7 4 0`, 600 frames, RAM: field people list (`*(*0x020AD7EC + 0x3E48)`, 0x2B0 each, count +0x3EAD) holds pid 137 with model 0. (The earlier queued-only test of 50402 worked, Sol was not drawn there either.) Likely the town model manager's job (`mq_town_models`) that covers the town only. |
| New Year in the dorm, 505 (`2001-01-02T10:00`, `MQ_VAR_WATCH=477`) | **works**: on entering the dorm each console runs 505.2 by itself, s477 0 -> 1 on both; nothing to share, no hang. |
| Halloween, 101 (Oct 29-31) | **not reached**: g1000 = 31 does not survive the host's town load in a session (below); with it poked after the load and 10101 queued, actor 211 is not created (not in the field's people list, nothing at (12, 82) at x 11.3-14.3, A says nothing). Grey sky in the pair, the purple Halloween sky in a solo control. |
| Test of courage, 501 (Aug 2-8; Mr. Vlad at (44, 10.5)) | **not reached**: its town set-up record (501.2) wants g1000 = 37, dropped by the town load (below); with 37 poked after the load and 50102 queued no teacher 122 appears. |

**The problem behind three of them: in a session the town load clears g1000** (the schedule's mystery, also on a date the
schedule holds). Repro: pair, `--clock 2000-12-24T10:00` (or `2000-10-30T10:00` with 31, `2000-08-03T15:30` with 37),
`var 411=0` on both, `var 1000=40` `var 1001=0` poked on the host (read back 40 from town block +0x2D8), then
`e 243` or `e 240` (a town load): g1000 reads 0 on both consoles. The real day roll does not help: from `2000-10-29T03:20`
with s411 = 0 poked before, the roll at 3:30 (visit ends, both reconnect) leaves g1000 = 0. Solo, same save and clock:
g1000 stays 31 / 40 after the same load (also with `MQ_COOP_SOLO=1`, so it is the game's own session check, not a hook).
`MQ_VAR_WATCH=0,1`: the write is `store[0] = 0` with lr 0x020CE88F, the field load's new-mystery code (ov001
0x020CE830..0x020CE8B8), where outside a session the schedule then picks the date's mystery again and in a session
nothing does. A map load that is not the town (Lovers Point, area 49) keeps g1000, which is why Christmas Eve works.
Every earlier mystery test poked g1000 after the town had loaded; by walking the host out of a building a real session
loses the dated mystery (Halloween, Santa in town, the haunted house of the test of courage, and presumably every weekly
mystery) at once. Not examined further (report only).
Left out of the sweep: the festivals' announcements before class (left out in a session already), collecting and
judging parts of the snow festival.

## The week's mystery in a session, and New Year's Eve's principal (2026-10-28, fixes fa9463f, 409c84f)

The two problems of the festival sweep above, fixed (docs/COOP.md, "The week's mystery is the calendar's on the host"
and "New Year's Eve: Principal Sol had no model"). Pairs of `day1done.sav` (AAAAAA, host) and `second_day1.sav`
(NORA, guest), same-PC link, Connect now, own binary copy; harness `/home/claude/run/live/gkpair.sh`, `gk_out.py`,
`gk_ny.py`; shots `/home/claude/run/live/shots/g1k/`.

- **Why g1000 went to 0**: the town's arrival check of the mystery (ov001 0x020C7DAC) skips itself in a session, both
  the drop of an ended mystery and the calendar's look-up; the load's other step (0x020C9DDA) drops an ended one
  anyway (here g1244 = 255: never started). Alone the look-up then started the date's mystery again (a reload with
  kind 2); in a session nothing did. Now the host's town runs it as alone.
- **Walking out of a building keeps g1000** (host into the bookshop, out of its door by the pad): Dec 23 / Dec 24 -> 40,
  Jun 25 -> 51, Oct 30 -> 31 from a save with no mystery, on both consoles; the town shows the mystery's sky in the
  pair (51's red, Halloween's). The sweep's repro (40 poked, `e 240`) keeps 40.
- **Not reached** (also not in a solo control with the same state): Santa at Lovers Point on Dec 24 21:00 after a fresh
  start of mystery 40 that day; the mystery-51 walk-in recipe (121.4) with the stage poked after the calendar's start;
  Halloween's ghost by day. These need the mystery's own progress through its week (stages from the day rolls), which
  a test that starts the mystery on its last days does not have.
- **New Year's Eve works**: Sol is drawn at the bell on both consoles; the host's and the guest's talks (504.2) run
  ("Ah! If it isn't NORA!"), the guest taken there by "Go along with AAAAAA?".

## Halloween's week through the day turns in a pair (2026-10-28, report only, no game code changed)

Pair of `day1done.sav` (AAAAAA, host) and `second_day1.sav` (NORA, guest), same-PC link, Connect now, binary of
f8e664c (the verify build with the week's mystery fix; an earlier copy of the build lacked it and showed g1000 = 0
after every town load, so its run was thrown away), `var 411=0` on both, harness `/home/claude/run/live/wkpair.sh`
(`wk_*.py`), logs `/home/claude/run/live/logs/wk_pair_h.log`, `wk_pair_g.log`, `wk_solo.log`, shots
`/home/claude/run/live/shots/wk2/`. The calendar (obj.dat, 0-based): only mystery 31 (Halloween, Oct 29-31) is dated
in late October; the weeks in between are rotation mysteries that need the one before solved.
- **Oct 29 03:26, the turn in the session works**: the visit ends, both reconnect (about 25 s), the date is Oct 29,
  g1000 is still 0 in the dorm. The first town load after it (A at the dorm door, then Down out of it, by pad; a warp
  `e 221` is not a town load of the game's own call and starts nothing) starts it: log "the host's town checked the
  week's mystery as alone (g1000 0 -> 0, the date's mystery starts)", then g1000 = 31, g1242 = 116 on both consoles;
  the host's sky is Halloween's. The guest's own screen kept the day sky until its own load (its g1000 reads 31).
- **Oct 30 (host and guest saved by the host's Disconnect, restarted at 03:24, the turn at 03:30 in the session)**:
  g1000 stays 31, g1244 0 -> 1, g1001 stays 0 (the day roll copies g1025, which is 0: Halloween's stage is moved by
  scenes, not by the roll), pumpkins appear at the dorm's door. Night actor 211 (the ghost) exists in the host's field
  people list from the town load on, jumping about the town every few seconds (positions from (9.6, 20.5) to
  (76.6, 51.6) within a minute).
- **Did not start / not reached**: (1) no Halloween scene started by real input on either day: none at stage 0 on
  Oct 29 by day (the graveyard (15, 84) and (12, 86) have nothing, solo neither) and the ghost could not be talked to:
  A beside it (host at (75.3, 52.1), ghost at (76.6, 51.6) a moment before) said nothing; a solo control (`wks`, same
  save and clock) chased the ghost the same way and got no scene either (it moves faster than the pad-driven chase, no
  contact in about 100 tries). (2) **Difference to solo**: in the pair the ghost's model reads 0 in the field's
  people list and the log repeats `coop town: model 4 asked for person 211 (not a classmate)` (16 times in about 2
  minutes), solo shows the ghost with a model (0x237c19c): in a session the ghost is probably never drawn, as
  New Year's Eve's principal was (fixed 409c84f); likely the same cause (the town model manager's request for a
  non-classmate does not stay: it is asked again and again). Not confirmed on screen (the ghost was never within the
  camera), so open. (3) "It's mystery time!" classmates (the rotation weeks) not reached: no week ended in these days.
- Time cost: 112 frames/s in a pair, 2400 frames per game minute: a day's turn needs a restart at 03:2x (saves from
  the host's Disconnect); waiting from 03:31 to morning would take about 50 min, and buildings are shut at night.

## Halloween night's ghost in a pair (2026-10-28, fixed)

The open item (2) of the entry above. Cause: the town's placement gives a person who is not a classmate model 4 and
lowers the people's count (field+0x3EAC 1 -> 0 in a session); our town model manager handed out model 4 regardless, so
the ghost (211) lost it and asked again whenever it roamed back into the camera's box. Fix in `mq_town_models`
(docs/COOP.md, "The town's own people keep their models"). Pair of `wk2/d2` saves (AAAAAA host, NORA guest), same-PC
link, `--clock 2000-10-30T03:28:30`, `var 411=0`, the day's turn at 03:30, the dorm's door in and out by pad (harness
`/home/claude/run/live/ghpair.sh`, `gh_dorm.py`, `gh_talk3.py`): the ghost holds model 4 on both consoles; set down
beside it (`a 211 1.3`), Up one frame, A: "Trick or treat?" with the Jack-o'-Lantern drawn; the guest got "AAAAAA has
found something mysterious!" / "Go and help AAAAAA?", A, and was taken there; both screens show the ghost and the
scene to its end (trick: "Abracadabra!" ... "Tah-tah!"). Shots `/home/claude/run/live/shots/gh/talk.png`,
`joined.png`, `after.png`. No fatal; QUICK PASS. Not run: the treat branch (item 441), Santa in town on Dec 23.

## Halloween by real input in a pair (2026-10-28, report only, no game code changed)

Pair of `day1done.sav` (AAAAAA, host) and `second_day1.sav` (NORA, guest), same-PC link, binary of 90cc515, `var 411=0`,
`a 211 1.3` (the student set down beside the ghost, then Up and A) for reaching it, harness copies in
`/home/claude/run/hw/` (`hwpair.sh`, `setup.sh`, `g_loop.py`, `st.py`), shots `/home/claude/run/live/shots/hw/`. How to
get there: `--clock 2000-10-30T03:24`, a town load by the dorm's door (starts mystery 31), the day's turn at 03:30, two
more door loads: the ghost then roams on both consoles with its model (g1244 = 1). Before that turn (game day Oct 29)
it stands at (12, 82) on the guest's people list only (the host's has no 211) and A beside it says nothing.

| What | Result |
|---|---|
| Trick (host leads, no item) | **works**: "Trick or treat?" then a Trick / Treat menu, A on Trick: "If you do not have any...", "Abracadabra!", "Heh heh heh heh...", "In the meantime, enjoy being me!", "Tah-tah!". **g1001 0 -> 1 on both consoles** at the end. The guest, not joined, kept its "Go and help AAAAAA?" open; Yes after the end: "Looks like AAAAAA has already worked it out." (no move, no hang). |
| Treat (host leads, item 441 poked into the closed bag, `20ade5c=441/2`) | **works, no hand-over needed**: "Very well then, I shall partake of your Candy!", "Heh heh! It has been a pleasure!", the candy leaves the bag by itself (no drag). **g1001 stays 0**; the ghost stays. A again (no candy) asks again and Trick then moves g1001 to 1. |
| Guest leads, Treat | **works**: the guest's A beside the ghost gives its own "Trick or treat?"; the host's screen "NORA has found something mysterious!" / "Go and help NORA?"; Yes took the host to (12.6, 44.9) beside the guest, its copy shows "Trick or treat?" (the host's own choice: A took Trick, it played its trick to "Tah-tah!"). Log "the guest's scene 10102 moves the town: (nothing); the host's joined copy undone": **g1001 stays 0 on both** (the guest's treat moves nothing). |
| Guest leads, Trick | **works**: same offer, host joined, both ran to "Tah-tah!"; log "the guest's scene 10102 moves the town: g1001=1 g1039=255 g1070=1 g1290=1 g1307=1; the host's joined copy undone": **g1001 = 1 on both**. |
| After stage 1 | The ghost's talk 10103 on the host: "......", nothing written, g1001 stays 1; on the guest 10103 wrote g1010 = 1, g1242 = 116. After the next day's turn (Oct 31, g1244 = 2) g1001 is **kept at 1** (a roll with the stage 1 saved by the host's Disconnect), the ghost stands parked at (120, 0) by day and by night (20:00), A has nothing to talk to; the haunted house door says "You are unable to enter." No further scene started by day or night on Oct 31 in the places tried (school door with pumpkins, dorm door, town); I did not find what moves stage 2 (the classmates' "mystery time" did not come). |
| End of the mystery | Turn at 03:30 on Nov 1: g1000 = 31 -> 0, g1001 -> 0 on both consoles, both reconnect and play (g1244 = 3), a weekly creature (176) appears. Clean: no fatal in any run. |
| The ghost's reach | At stage 1 the ghost moved off whenever the student was set down beside it; only 3 of about 400 tries (host) got a talk and one was the haunted house door; the guest got none in 250 (its stage-1 chase). At stage 0 the first try worked for both. |
| Harness caveat | A pair warped (`e 221`) while the guest was still loading its town after the day's turn left the guest on a black loading screen for 10000+ frames (not reproduced with 900 frames of waiting; not reachable by real input). |

Open: what moves Halloween past stage 1, the classmates' week, and the guest's stage-1 talk (10103 wrote g1010 there only).

## Halloween's progress, the ghost on the first day and at stage 1 (2026-10-28, fixes 11bd4a9, cfcf93e)

**How Halloween goes (for players, alone or together; Oct 29-31).**
1. The Jack-o'-Lantern turns up in town the day **after** the mystery begins (the game's own rule: nothing of a
   mystery is placed on the day it starts). Pumpkins mark the doors.
2. Walk up to him and press A: "Trick or treat?". **Treat** with a Candy: he eats it (two candies earn a present).
   **Trick**: he casts "Abracadabra!" and you take his looks.
3. Back in town he laughs "You will have to catch me if you wish to return to normal!" and is gone.
4. **Talk to any classmate** (they answer your trick-or-treat). That brings him back at one of three spots: two on the
   high ground at the north edge of town (one by the gravestones and the waterfall), one at the town's west side.
5. **Find him and press A**: "Egad! How did you know it was me?!" He turns you back, and next "Take this as a sort of
   reward!" (a present). The mystery counts as solved; he stays for more trick-or-treat until Halloween ends (Nov 1).
He wanders and turns away when you come close: go to him soon after entering the town. Together: whoever does a step
moves Halloween on for both, the other is asked "Go and help?".

Pair of `day1done.sav` (AAAAAA, host) and `second_day1.sav` (NORA, guest), same-PC link, Connect now, binary of the
two fixes, `--clock 2000-10-30T10:00`, `var 411=0`; harness `/home/claude/run/hw2/` (`pair.sh`, `run2.py`, `run3.py`,
`lib2.py`; solo controls `solo.sh`), shots `/home/claude/run/hw2/s7.png`, `end.png`. Details: docs/COOP.md,
"Halloween (mystery 31, event file 101) and the day's flag".

| What | Result |
|---|---|
| What moves the stages (decoded, then played alone by real input) | 0 -> 1: Trick at the ghost (101.2). 1: the next load's 101.3 hides him. 1 -> 2: any classmate's talk (101.4). 2 -> 3: A at the ghost (101.6). 3 -> 0: the next load's reward scene (101.7). Alone all four by A presses (set down beside the ghost, `a 211 1.0`); no dated night scene, no place or item needed beyond Candy for Treat. |
| Host's ghost missing before the first turn | **The game's rule, the guest was wrong**: a mystery's start sets the day's flag (0x020AD7FE, field+0x3EB0) to 1 until the next day, and the ghost's start record wants 0; alone: no ghost on the start day (three loads), at once with the byte poked to 0. The guest never runs the start and kept 0. **Fixed (11bd4a9)**: the guest takes the host's byte; now no ghost on either on the first day, both have him the next. |
| Stage 0 -> 1 in a pair (host, Trick) | works, stage 1 on both, "catch me" scene. |
| Stage 1 ghost | **Co-op only, fixed (cfcf93e)**: alone he is off the map at stage 1 (nothing to talk to); in the pair the host's copy followed the guest's (NPC owner: the nearer student) and walked the town (the earlier "3 talks in 400", one of them 101.3 again). Now at (120, 0) on both for good. |
| Stage 1 -> 2 in a pair | works: host talking to classmate 6 ("TREAT!"), and in a second run the guest talking to classmate 79: stage 2 on both. |
| Stage 2 -> 3 -> 0 in a pair (host) | works: first try after a fresh load, "Egad! ...", reward scene, stage 0 on both (the guest's after its "Go and help?" closed). |
| The ghost's evasion | **The game's**: alone 1 talk in 22 and 1 in 85 set-downs at stage 0, 0 in 160 at stage 2 with a chase; first try right after a load (alone and in the pair). |

No fatal; QUICK PASS. Not run: the treat present (two candies), the day's-end reset (101.98), the guest catching him.

## The weekly mystery of Nov 24-30 (39) and Santa in town on Dec 23 by real input in a pair (2026-10-31, report only, no game code changed)

Pairs of `day1done.sav` (AAAAAA, host) and `second_day1.sav` (NORA, guest), same-PC link, Connect now, binary of
ccb7557 (the verify build), `var 411=0` on both, harness copies in `/home/claude/run/nov/` (`pair.sh`, `nl.py`,
`nl2.py`; the `hw2` drivers), shots `/home/claude/run/nov/santa*.png` (`santa9.png`: both students at Santa).
Decoded first (script decoder, event files 109 and 110, the start tables, `obj.dat`).

**Mystery 39 (event file 109, Nov 24-30), decoded.** Its stages are g1001 0-3. Stage 0: nothing is placed in town at
all (only the creature catalog's actor 219 at (3, 0) on maps 71 and 44, events 070.39 / 258.39); block 6 (town record
141, needs the day flag 0) sets g1191 = 1. Stages 1-2: act_u records 380 (stage 1) and 381 (stage 2) run block 3
(stage 2, g1011 + 1) when a cast of those numbers happens with t164 = 2, t165 = 7; block 2 adds to g1010 and at 5
moves to stage 1 (g1191 = 3), block 4 takes g1011 down: **no start record, no other event and no script command
starts blocks 2 and 4** (the start tables hold only five records for 109), so the game's code starts them; I did not
find what. Stage 2 with g1011 >= 1: loading Fungi Forest (map 66, mc_ss block 66) runs block 5: a spirit (actor 219)
is summoned at (5.5, 5), "Why, you were able to summon me... Very impressive ... we can only reveal ourselves to you
once", stage 3, the mystery's done mark (query 0x1F(39)) and a query 0xBA(1) (the reward). Block 99 (Dec 1) clears it.
Related: the flatulence lesson (event 021, lesson 41, "break wind on a ghost, you can blow them away") teaches
incantation + *toilet* (WHERE tab, page 2, row 3 col 5) + *performance* (DO tab, page 1, row 3 col 3), charm record 30
(event 381, "Flatulence magic was successfully added to the staff").

| What | Result |
|---|---|
| Mystery 39 starts (pair, `--clock 2000-11-24T10:00`, dorm door in and out by pad) | **works**: g1000 = 39 on both, the day flag (0x020AD7FE) 1 on both ("the day's flag 0 -> 1 (the host's)"), g1191 0, nothing new in town. |
| The next morning (the flag poked to 0 on the host, a door load) | **works** as the game's rule: flag 0 on both, g1191 = 1 on both (block 6), still nothing visible or to talk to (no actor above 150 on either console). |
| The steps (what moves g1010, the cast, Fungi Forest's summon) | **not reached**: I did not find what starts blocks 2/4. Casting incantation + toilet + performance in wizard form (solo, day 2: transform by the D-pad, Y, the three words, OK; the words screen works) ends silently: no text, no scene, g1010 / g1011 unchanged (the day1done student probably does not know the spell: it is taught in lesson 41, "added to the staff"). So no reward and no pair test of the stages; nothing is known to be wrong in a session either. |

**Santa in town on Dec 23 (mystery 40, event 110).** Decoded: 110.1 (town record 142, stage 0, day flag 0) puts Santa
(actor 220) at (66, 10.5) only if **g1322 is not 0 and g1010 < 2**. g1322 = 1 is written only by 118.4, the last
purchase of the Matchstick Girl's mystery (48, Dec 18-21: four matches bought from her, g1002 = 4). So a student who
never helped her does not see Santa in town on the 23rd. His talk (110.2) wants a match (item 120): "Ho ho ho! Have
you been naughty or nice?", asks for a match, "Oh, thank you...", the Matchstick Girl's thanks, g1010 = 2, g1322 = 0,
Santa goes; without a match "Oh, you don't have any, eh?" and he stays (g1010 = 1). Played in a pair with the
precondition **poked** (not real input: `var 1322=1` on the host, a match `20ade5c=120/2` in the host's bag,
`--clock 2000-12-23T10:00`, the start day stood in for by poking the flag to 0, then a door load):

| What | Result |
|---|---|
| Santa in town | **works**: actor 220 on both consoles at (66.0, 3.0, 10.5) with a model (the earlier "not reached" was the g1000 loss, fixed). |
| Host walks up, A | **works**: "Ho ho ho!", scene 11002 (log `scene 11002 (mystery 40 stage 0) started here`), the match handed over, the talk to "Remember, I'm gonna find out who's naughty or nice", g1010 = 2 on both; g1322 read 3 afterwards (poked 1; the byte's meaning not examined). |
| The partner asked to join | **works when answered in time**: "AAAAAA has found something mysterious!" / "Go and help AAAAAA?", Yes: the guest stands beside the host and runs Santa's talk too (no match: "Oh, you don't have any, eh?"). **Harness caveat**: the offer lasts 3600 frames of the guest; the unpaced harness runs the idle guest faster than the host, so twice the answer came after "(too late)" and the guest heard "Looks like AAAAAA has already worked it out." while the host's scene still ran. At 60 fps a player has a minute; a slow answer gives the same text. |
| Guest leads | not run (Santa was gone after the first talk). |

**How it goes, for players.**
Mystery 39 (Nov 24-30): superseded by the entry "The fairy (mystery 39) ... 2026-10-31" below (it is about fairies,
not the flatulence spell; the spell's words are *magic* + *toilet* + *performance*, a staff charge for the haunted house).
Santa (Dec 22-25):
1. Help the Matchstick Girl in her week (Dec 18-21: buy four matches), or Santa does not come on the 23rd.
2. From the morning after the mystery starts (Dec 23) Santa stands at the north edge of town at (66, 10.5) between two street lamps; keep a match.
3. Walk up and press A; give him the match.
4. The partner is asked "Go and help?": answer within a minute.
5. His present comes on Christmas Eve at Lovers Point (verified before).

Not done: the Matchstick Girl's week by real input (Santa's precondition was poked), mystery 39's steps, Dec 24's present.

## The fairy (mystery 39), the Matchstick Girl (mystery 48) and Santa on Dec 23, by real input in a pair (2026-10-31, fixes in docs/COOP.md "Mystery 39 (the fairy ...)")

Pairs of `day1done.sav` (AAAAAA, host) and `second_day1.sav` (NORA, guest), same-PC link, Connect now, own binary copy,
`var 411=0` on both; days moved on by the host's Disconnect (both save) and a restart at the next date (harness
`/home/claude/run/m39/`: `pair.sh`, `m39lib.py`, `pa*.py` (the fairy), `d*.py`, `dg.py`, `dsave.py` (the matches,
Santa); shots there: `pn5.png` both at the fairy, `santa_pair.png`). Decoded first: text messages start events with
`$073'<file>'<block>;`, which is how the blocks no start record names (109.1, 109.2, 118.2-4) begin.

**How the fairy goes (mystery 39, Nov 24-30), for players, alone or together.**
1. From the day after it begins, Miss Strawe is looking for you (classmates say so). She is in the school's staff
   room (on the days she is on duty there). Talk to her: she wants the whole class to believe in fairies.
   After her last line wait a moment before pressing A again (an A at once starts her talk over and the request does
   not count).
2. Talk to classmates. A doubter asks "Do you agree with me?" / "...they don't exist, right?" (answer **No**) or
   "Fairies do exist, don't they?" (answer **Yes**). Every such answer counts, one per classmate a day; five, and she
   is satisfied ("Oh, great job! ... lay out a White May Lily as proof ... while it's still light").
3. Take a White May Lily to Fungi Forest by day and put it on the ground there (drag it out of the bag to the ground
   in front of you). Nothing is shown; it counts.
4. Come back to Fungi Forest at night (19:20 worked; 14:00 did not): the fairy appears among lilies, talks, and gives a
   present. Done.
Together: whoever does a step moves it for both; the steps with nothing to see (her request, a classmate won over, the
lily) are not offered to the other; the fairy is ("Go and help?"), or the other just walks in too.

| What | Result |
|---|---|
| Alone, each step by real input (Nov 24, the start day stood in for by poking the day flag to 0) | Strawe's request (g1070 = 1), classmate 68's question answered No (g1010 + 1), the lily laid in Fungi Forest (stage 2, g1011 = 1), the forest at 19:20: "Why, you were able to summon me...", stage 3, a present. Stage 1 and the lily item were poked (`var 1001=1`, `20ADE5C=207/2`); the lily is a precondition item. |
| Strawe in the pair | **Fixed**: she had no model in a session (not drawn, no talk). Now drawn; the host's talk ran 109.1, g1070 = 1 on both, the guest **not** asked (before the fix it got "Go and help?"). |
| Persuasions in the pair | Classmates' doubting question did not come up in 4 rounds of both students talking to the three classmates out that evening (it is the game's random topic; alone classmate 68 asked twice in 8 rounds). Counted with 109.2 queued (as the text tag does): guest, host, guest, host, guest: 1..5 on both, stage 1 from the guest's fifth, nothing offered. |
| The lily in the pair (guest, by pad and touch) | stage 2 on both ("the guest's scene 10903 ... moves the town: g1001=2 g1011=1"); the guest's matchers had been set to stage 1 with no event of its own (the fix). |
| The fairy in the pair (19:20, saved and restarted) | host's load of Fungi Forest: the fairy; guest asked "Go and help AAAAAA?", its Yes came too late in the harness ("Looks like AAAAAA has already worked it out."), then it walked in itself: both saw the fairy at once (`pn5.png`), stage 3, each a present. |
| Crash found on the way | the host ended once (`no recompiled function at 00000000 (lr 02031c7f)`, the guest's ground-object packet with its copy's model gone, the guest set down beside classmates one after another): **fixed**, passed over twice since. |

**How the Matchstick Girl goes (mystery 48, Dec 11-17 by the calendar, `obj.dat`; not Dec 18-21).**
1. Nothing the first day. From the next day she stands in town in the evening: day 1 of her stall at the end of a
   bridge (44, 77), day 2 at (62.5, 62), day 3 on the high ground at the north edge (68.5, 7; walk over from
   Harrows' side, not from below the cliff).
2. Talk to her, "Buy a match?" Yes: 100 Ritch, one match each talk. Two the first day, three the second, four the
   third; after each day's last one she goes home for the day. The last one ends the mystery ("I'm going to hurry over
   to my..."), with a present, and marks you as her helper for Christmas (g1322).
3. Keep a match: Santa asks for one.
Together: each buys from her on their own (not offered to the other); both students' purchases count toward her day.

| What | Result |
|---|---|
| Dec 11 | mystery 48 starts on both (g1000 = 48), nothing placed (the day flag). |
| Dec 12, 13, 14 (restarts at 18:30) | the guest bought 2 + 3 + 4 by real input; stage 0 -> 1 -> 2 -> 3 on both, her place each day as above; the host was neither pulled into nor asked about a purchase (before: standing near, it was put into the guest's purchase at once). g1322 = 1 on both (a bit flag: store +320 + 8, bit 2; the "3" in the entry above read the wrong byte). |
| The host buying | not reached: by the harness's set-down the host stood in the river below her (day 1) or below the cliff; the guest reached her by walking. The host's purchases were not counted together with the guest's in a run. |
| Santa, Dec 23, no pokes (Dec 22 load starts mystery 40, saved; Dec 23 10:00) | Santa at (66, 3, 10.5) on both. The guest walked up from Harrows' side, A: "Ho ho ho! Have you been naughty...", gave a match, "Are you the one ... By helping her sell ... She said she'll be forever...", "Remember, I'm gonna find out..."; g1010 = 2 on both. The host was asked "Go and help NORA?" (not answered; the offer ended with the scene). |

QUICK PASS. Not run: the host's own purchases; a classmate's doubting question in the pair by real input; Santa's
present on Christmas Eve after this; the lily laid at night.

## The host's matches (Dec 12-14) and Santa's Christmas Eve, by real input in a pair (2026-10-31, report only, no game code changed)

Pairs of `day1done.sav` (AAAAAA, host) and `second_day1.sav` (NORA, guest), same-PC link, own binary copy of the
tree's build (a91b2e4), days moved on by the host's Disconnect (both save) and a restart at the next date (18:30, then
Dec 22 10:00, Dec 23 10:00, Dec 24 20:30; `var 411` as in the entries above). Driver scripts and shots in
`/home/claude/run/xmas/` (`s3.py`, `s4.py`, `s5.py` the purchases; `s6.py` Santa on the 23rd; `s7.py`, `s8.py` the 24th;
`santa24.png`). Decoded first (event file 110, `mqscript.py`).

**Where to stand for the stall (host).** Arrival points drop the student near her without the river: `e 238`
(57.0, 58.75) then walk about 18 units to day 1's stall at (44, 77) (`goto` took 150 steps, arrived at (45.3, 77.1));
`e 239` (62.3, 58.6) is 3.5 units from day 2's stall (62.5, 62); `e 233` (73.2, 7.8) is 5 units from day 3's stall
(68.5, 7). `e 243` (31.9, 72.7) lands on the wrong side of the river for day 1 (stuck at a tree and the bank).

| What | Result |
|---|---|
| Host buys on Dec 12 (2 matches, guest in town at (62, 58)) | **works**: stage 0 -> 1 on both consoles; the guest was not asked, not moved, no text. |
| Dec 13 (3 matches) | **works**: stage 1 -> 2 on both. The host's own g1001 read 1 for a second after the guest's side read 2: it is the 1/s town sync, both 2 a second later. |
| Dec 14 (4 matches at (68.5, 7), from Harrows' side) | **works**: stage 2 -> 3 on both, "I'm going to hurry over to my...", 9 matches in the host's bag over the three days, the guest's bag empty (it bought none). The guest log showed the matchers flip 2 -> 3 -> 2 -> 3 once (below). |
| Santa Dec 23, no pokes, host bought all, guest none | Santa on both consoles at (66, 3, 10.5). The guest (no match) talked first: "Oh, you don't have any, eh?", g1010 = 1 on both, he stays; then the host (a match): scene 11002 to its end, g1010 = 2 on both, one match gone. The guest standing beside Santa made the host's A start a chat with the *guest* instead (harness: move it away first). |

**What Dec 24 does (event 110, decoded).** Not a town event: Lovers Point (area 49, mc_ss block 49 record 9) at night (hour
<= 4 or >= 19) while g1010 <= 5 or the present is out (g1024 = 1) puts Santa at (7, 1.5) (block 11, g1001 = 1). His
talk (block 12): he lost the map to the nice children's houses; "Will you help me?" Yes: he hands over **one present
(item 123)** to be put on the ground in front of a child's house (12 houses, block 1 lists their ground spots, block 14
on map 50 counts the deliveries); g1010 = 6, g1011 = 1 (a present is out), g1007 -1 (presents left; set to the total
the first time). While g1011 = 1 he only says "Once you deliver the previous present I gave you...". When
g1002 (given out) equals the total: the thanks, a special present (item 397, put on the ground if the bag is full),
g1001 = 9, g1392 = 1 and the mystery's done mark and reward (queries 0x1F(40), 0xBA(2)). All before sunrise; the 25th
night also works.

| What | Result |
|---|---|
| Host alone at Lovers Point, Dec 24 20:30 + (`49 7 4 0`, Santa at (7, 0, 1.5) on the host) | **works**: "Will you help me?" Yes, item 123 in the host's bag, g1010 = 6, g1011 = 1, g1024 = 1, g1007 = 11 on both consoles; the guest got the offer; answered too late in the first run ("the offer ... is gone (too late)", the harness's fast idle guest). |
| Same, the guest answers the offer in time (Yes) | **works**: both walk through his talk (`santa24.png`, both screens "Once you deliver the previous present..."), **both bags hold item 123**, g1010 = 6 / g1011 = 1 / g1024 = 1 / g1007 = 11 on both. |

**Observations (not proven bugs).**
1. Only one present is counted for two: g1007 went 12 -> 11 once although two presents were handed out, and g1011 is one
   shared flag. A student who does not join the scene gets none until the present out is delivered (his talk says "Once
   you deliver the previous present I gave you"). Whether the pair can then finish (g1002 reaches the total with two
   presents in circulation) is **not reached**.
2. The guest's stage matchers flipped 2 -> 3 -> 2 -> 3 right after the host's last purchase (Dec 14); the town values
   settled at 3. Harmless in this run; likely the guest's town copy briefly reading the old stage between the host's
   diff and its own result write.
3. The Gateway room (where the guest idled) shows the offer text but the text log only reads the player name
   ("AAAAAA"): the offer's "Go and help AAAAAA?" can be waited for by that line.

**How it goes, for players.** Matchstick Girl, together: each buys from her alone; the host's purchases count for both
(Dec 12 two, Dec 13 three, Dec 14 four, 100 Ritch each) and move her stages on both screens; the partner is never
pulled in. Keep a match, talk to Santa on the 23rd (north edge, (66, 10.5)); on Christmas Eve after dark go to Lovers
Point, talk to Santa, say Yes: whoever stands by him when the other answers "Go and help?" with Yes gets a present too.

QUICK-level run only. Not run: delivering the presents (the houses' ground spots, 0x6E / g1310-1321), the finale's special
present, Santa on the 25th, the walk from town to Lovers Point (the host was warped there, as in the earlier Christmas Eve test).

## Santa's Christmas Eve presents: decoded, the delivery not reached (2026-10-31, report only, no game code changed)

Pair of the Dec 23 saves of the entry above (`/home/claude/run/xmas/s/d23`, AAAAAA host, NORA guest), same-PC link
`xmas2`, own copy of the tree's build (9b298a2), `--clock 2000-12-24T20:30`; then solo copies of the host's save for
the delivery attempts. Harness and shots in `/home/claude/run/xmas2/` (`pair.sh`, `s7.py`, `s8.py` Santa, `holdscan.py`,
`drag2.py`, `sweep.py`, `lily.py` the delivery attempts; saves of the night with both presents in `s/d24a`). Decode:
docs/COOP.md, "Mystery 40, Santa's Christmas Eve".

| What | Result |
|---|---|
| Host talks to Santa at Lovers Point, guest answers "Go and help AAAAAA?" Yes | **works** as before: both walk through the talk, both bags hold item 123, g1010 = 6, g1011 = 1, g1024 = 1, g1007 = 11 on both (`santa24.png`). |
| g1007 = 11 after one present | **the game's rule**: on the first present g1007 is set to the total of lived-in homes (query 0x6F), 11 of 12 in this town; nothing was lost. |
| Delivering at house 3 (tree (47, 54), door about x 49.5, z 54): the present dragged from the bag to the ground in front, beside, behind, onto the door, onto the tree; held up and A; A at the door | **not reached**: a drag onto the ground goes back into the bag (the present lacks the bag's "can go on the ground" bit), a drag onto the student holds it up, A puts it back; g1002, g1011, g1310-1321 unchanged. The same alone (so not the session). |
| Saved by the host's Select save, restarted | the guest's present was gone (`s/d24a/guest.sav`); not traced. |

**How it goes, for players (as far as reached).** On Christmas Eve or Christmas night after 19:00 go to Lovers Point
and talk to Santa; say Yes and he hands over one present. If your partner is elsewhere they are asked "Go and help?";
Yes brings them over and they get a present of their own. Santa gives the next one only after the one out is
delivered, whoever holds it. How a present is delivered in front of a child's home was not found by play or by reading
the code; the finale (a special present, the mystery's mark) and Dec 25 were not reached.

Not done: the delivery and everything after it (the count, the finale for each student, Dec 25), so no co-op fix was
made; the guest's present gone after a save; Santa drawn dark with speckles on the host's screen at Lovers Point.

## Santa's presents delivered, and the finale, by real input in a pair (2026-10-31, report only, no game code changed)

Solo copy of `/home/claude/run/xmas2/s/d24a/host.sav`, then a pair of the d24a saves (AAAAAA host, NORA guest, same-PC
link `xmas3`), own copy of the tree's build (d4700e6), `--clock 2000-12-24T21:00`; then the pair's saves of the end
(`/home/claude/run/xmas3/s/d24done`) at `2000-12-25T21:00`. Harness and shots in `/home/claude/run/xmas3/` (`start.sh`,
`pair.sh`, `x3.py`). Decode: docs/COOP.md, "Mystery 40" (the delivery is the door script, event 253 block 43).

| What | Result |
|---|---|
| Solo, house 3: stand square in front of the door (49.6, 54.1), facing it, A | **works**: "Put the present here?" (`put1.png`); Yes: the present from the bag to the ground before the door (`put2.png`), g1002 0 -> 1, g1011 1 -> 0, store index 317 = 1 (home 8). Half a unit to the side, A finds nothing (the earlier failures). |
| Pair, host delivers at house 3 | **works**: g1002 = 1, g1011 = 0 on both; the guest is not asked or moved. |
| Guest (its present lost earlier) to Santa, Yes | **works**: present into the guest's bag, g1011 = 1, g1007 10 on both; the host is offered "Go and help NORA?" (No). |
| Guest A at the door of marker (60, 40) | "Nobody lives here." (home 12, the one not lived in). |
| Guest delivers at marker (20, 64), stand (21.6, 66.1) | **works** (`p_g2.png`, `p_g3.png`): the host runs the guest's writes, g1310 = 1, g1011 = 0, g1002 = 2 on both. |
| `var 1002=10` poked (the 8 other homes skipped), host's next present, delivered at marker (28, 88), stand (29.6, 90.1) | **works**: g1002 = 11 = the total. |
| Guest talks to Santa, host answers "Go and help NORA?" Yes | **works**: both run the finale to "Merry Christmas!", **item 397 in both bags**, g1001 = 9, g1070 = 2, g1024 = 0 on both, Santa leaves (`finale.png`). |
| Host's Disconnect (both save), restart Dec 25 21:00 | item 397 kept in both bags, g1001 = 9; Lovers Point without Santa, the town's trees still up (`d25.png`). |

**How it goes, for players.** After Santa hands you a present, walk to a classmate's home at night, stand right in
front of its door, face it and press A: "Put the present here?" Yes. Then back to Santa for the next one (he gives one
at a time for the town, whoever holds it). In a pair each delivers on their own; the count is shared. When every home
has one, talk to Santa: if the partner answers "Go and help?" Yes, both get his special present.

QUICK-level run only. Not run: the 8 other deliveries by real input (poked), Dec 26 (the trees away, block 97, and the
mystery's end, 99), the done mark (query 0x1F(40)) read back for each student, the dark Santa on the host's screen
(not seen in this run), the guest's present lost after the earlier save.

## Santa's presents, one per student, by real input in a pair (2026-10-31, fix in docs/COOP.md "Mystery 40")

Pair of `/home/claude/run/xmas2/s/d24a` (AAAAAA host holding a present, NORA guest), same-PC link `xmas4`, own copy of
the tree's build with the change (`game/src/coop_myst.cpp`, `mq_santa_present`), `--clock 2000-12-24T21:00`; then a
pair of the saves of its Disconnect (`/home/claude/run/xmas4/s/disc1`). Students placed by `MQ_MYST_GOTO` (an arrival
point, then a step beside a door); every talk, answer and delivery by pad. Harness and shots in `/home/claude/run/xmas4/`
(`pair.sh`, `santa.py`, `deliver.py`, `place.py`, `ans.py`).

| What | Result |
|---|---|
| Host delivers at (21.6, 66.1) | **works**: g1002 0 -> 1 (`h1_put.png`). |
| Host talks to Santa, guest "Go and help AAAAAA?" Yes | **works**: both bags hold item 123 (`santa1.png`); each asking again: "Once you deliver the previous present...". g1007 11 -> 10. |
| Host's Disconnect (both save), the guest's save loaded alone | the guest's **save holds item 123** (save +0x9960 + 0x636), the loaded bag none: **the game's rule** (a present is taken at load from a student whose own town is not at mystery 40: NORA's g1000 = 0); the host's save, its town at 40, keeps it. Not co-op. |
| Pair of those saves: the guest (none now) talks to Santa while the host holds one | **works**: a present for the guest (the one flag would have refused), g1007 9 (`santa2.png`). |
| Guest delivers at house 3 (49.6, 54.1), then the host at (29.6, 90.1) | **works** (`g2_ask.png`, `g2_put.png`, `h2_ask.png`, `h2_put.png`): g1002 1 -> 2 -> 3, each once; g1011 stays 1 while the host still holds its present, 0 after. |
| `var 1002=10`, `var 1310=0` poked (the other homes skipped); host talks to Santa, guest joins | **works**: the host gets the last present; the guest is refused ("Once you deliver...": its read 1, homes 11, delivered 10, the other's present counted), so no spare (`santa3.png`). |
| Host delivers it at (21.6, 66.1) | g1002 = 11 = the homes (`h3_put.png`). |
| Guest talks to Santa, host "Go and help NORA?" Yes | **works**: the finale for both, **item 397 in both bags**, g1001 = 9, g1070 = 2, g1024 = 0, **no item 123 left in either bag or hand**, Santa gone (`finale.png`). |

**How it goes, for players.** In a pair each of you can hold one of Santa's presents and deliver it at any home's
door yourself; Santa gives you another once yours is delivered, whatever your partner holds, but never more than the
homes left. The count is shared; when every home has one, talk to Santa together for his special present. A guest's
present goes away if the guest goes home and loads again before delivering it (as the game has it); ask Santa again.

QUICK PASS on the binary. Not run: Dec 25 and 26, the solo game after a session with a present out (the store's g1011
kept from the bags: written once in the pair, not played alone afterwards).

## Soak 3: internet mode at 200 ms, three segments, about 75 minutes (2026-10-31, d3f33f8)

Binary of d3f33f8 (code of a91b2e4), `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`, host `day1done.sav` (AAAAAA), guest
`second_day1.sav` (NORA), the soak driver adapted (`/home/claude/run/soak3/`). Segment 1 (Jul 18, 14:00-14:49, 33 min):
town together and apart, a chat each way, a gift each way, both into the bookshop and out (the clerk's talk), homeroom
and a lesson, the host's Disconnect (both "Save complete."). Segment 2 (Jul 19 from 02:55, from those saves): connected,
the day turned in the session at 3:00 and both were connected again 30 s later, a walk. Segment 3 (Jul 19 from 10:00, from
segment 2's saves): the dorm in and out, the school door, the host's Disconnect and save. 54 state comparisons, no fatal
line, no stall over 3 s, no frame-rate drop; resident memory flat (main 340.8 -> 342.9 MB over 33 min, stand-in 349.5 MB
unchanged; 341.2 -> 341.5 MB in segment 3); town block, clock, date and `.coop` records equal at every check where both
consoles were in the session. No black screen in the shots read (top screens show the sky/dorm room, the town below).

**Seen, not understood or not a fault**
- *Friendship at the ruins failed* (segment 1): both transformed, the guest chanted, the host chanted with the emerald
  and got "You failed at using ..." (host log frame ~167,240; `.coop` stays stage 0, friendship 7; `shots/cast/soak1fr_8.png`).
  The earlier soak (2026-10-27) succeeded with the same driver; this run had another pair running (load 5-7, 65-80 frames
  a second per copy). Not retried: possibly the driver's timing against the game's wait, not checked.
- *Remote copy's position stale in the dorm* (segment 3, `dorm_in`): both walked in from the arrival point (Up, A); 4 s
  after, the guest stood at (14.72, 8.32) on its own console while the host's copy of it was at (15.0, 12.5), act 0, and
  the host's own place was (15.0, 10.5), so the two screens showed the students in the opposite order
  (`shots/soak3/d3_18_dorm_in.png`). Back in town the copies matched. Suspect: the last movement packet of the guest
  after the field load/stairs not reaching the host's copy (coop movement path).
- *"copy on the other console is not here"* reported ten times in segment 1 while the guest stood ~15 units from the
  host: the remote's model pointer (+0x20) was 0 at that distance. Looks like the checker's false positive (no draw
  object beyond a distance), the same check passed when the students were close.
- *School door* at 10:00 on a Wednesday: three tries by real input each, neither student got in, no text, for both
  (as at 03:00). Not compared with a solo run, so not known whether the door is shut by the game (summer) or refuses in
  a session.
- The guest stuck for ~10 minutes against the cliff at (39-40, 42-43) in segment 1 (the driver's route; both
  students stand and A/pad worked after warps): a driver problem, not checked in game.

**Not covered**: the café (the driver's two walkers timed out; the host's walk ended at a house door with "You had a
nice chat with NORA." from the guest passing, `shots/soak3/d3_17_cafe.png`), shop purchase, a lesson on day 2, fishing and
insects (left out for time), a talk with a classmate on the second day.

## The school's door and the dorm's stale copy (2026-10-31, fix in game/src/coop_talk.cpp `arrival_end`)

From soak 3's open items, pair of the soak's own saves (h_d2.sav AAAAAA host, g_d2.sav NORA guest), `--clock
2000-07-19T10:00`, own folder `/home/claude/run/door/` (driver `dorm.py`, `school2.py`), link `door1`.

- **School door at 10:00 on Wednesday Jul 19**: solo (`MQ_COOP_SOLO=1`, the same save) the soak's way (to (43.9,
  27.4), Up 10, A) failed three times, no text, ending at z 26.5-26.7; from the arrival point (44.0, 23.91), Up 6, A:
  inside the hall (12.0, 18.9). The school is open; the drivers stood three units short (camera-point coordinates
  from before 298977a). In the pair over the internet (200 ms / 1 %), by pad from (44.0, 24.6): the guest in, then
  the host in, both in the hall (area 6) with each copy where the other stood (`/home/claude/run/door/dh/school_in.png`).
  Earlier tries in the same pair were spoiled by the driver: A next to the other student opens the player talk.
- **Stale copy after entering the dorm** (both in from the arrival point 221 one after the other, Up, A):
  reproduced on the **same-PC link** at once and for good: the host's copy of the guest in action 7 (the walk through
  a door) at the door (15.0, 12.5) while the guest stood at (14.72, 8.32); minutes later, the host and the guest
  both walking about, it still stood there (back in town it matched). The copy's packet queue held the guest's
  standing 0x0F at (14.72, 8.32), which a copy in action 7 ignores (main 0x02031404's 0x0F case); the walk ends only
  with the guest's 0x13 back to 0, one frame of shared bytes, missed when the consoles' frames do not line up. Over the
  internet the stand-in's safety net (5 s, then the stand-in's bytes until it stands) hid it for ~8 s (soak 3). Fix:
  a copy in action 7 for 1.5 s while its console's kept bytes (session +0x4A08 + slot * 0x50) say the student stands in
  this area over half a unit away is set back to 0 (main 0x0201E4DC, as the 0x13 would); its movement then takes it to
  the student ("coop talk: slot 1's copy was left in the walk through a door here ... ended"). Verified by real input:
  same-PC link, the copy at (14.72, 8.32) act 0 at the first sample, both screens in the same order
  (`/home/claude/run/door/dh/dorm_fixsame.png`, `dg/dorm_fixsame.png`); internet mode, three rounds into the dorm, the copy right at
  every sample (once by the fix, ~1.5 s), out and back in town fine. QUICK PASS.

## Internet mode at 200 ms: Friendship, the café, shop purchases, Christmas Day and Dec 26 (2026-10-31, report only, no game code changed)

Binary of 45da7ca (its fix present), `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`, `--coop-internet`, own folder
`/home/claude/run/net4/` (soak driver `soak.py` and the libraries copied there, link `net4`). Host `day1done.sav` (AAAAAA),
guest `second_day1.sav` (NORA); every step by pad. Money was poked once (a mistake: the write went to 0x020AD828, not
the money at 0x020ADEC4; both students started with ~3000 Ritch from their saves anyway).

| What | Result |
|---|---|
| Friendship on the other student at the ruins (`soak.py 1 7 friendship`, machine quieter, 65-70 frames a second) | **works**: both transformed, the guest chanted, the host held the emerald out: "You succeeded in using..." (host), `coop pair: Friendship reached slot 1 (+2)` and "Friendship from the other student (slot 0 ... friendship 2)" on the guest; `.coop` stage 0, friendship 2; the guest's bag keeps its emerald, the host's emerald was used. The soak3 failure did not repeat (it ran at load 5-7). |
| Café at 18:00 (evening) and at 13:00, both in from the door by pad | enter **works** (both in at once, each gets Marcy's "Oh? We haven't met, have we?"); the players' own talk works inside (A beside the other: "You chat with NORA about classes"); leaving **works** (the guest from x 5.0 only; from x 7.5 Down does nothing). **Ordering does not work: after the introduction, A at Marcy gives "Hello! Come on in." / "I'm sorry, dear, but we're not open yet." on both, at 18:09 and at 13:00.** Solo (same binary, same save, 13:00, no session): "Would you like something ...?", "It costs 200 Ritch for a glass of juice" (`shots/solo_b.png`). So the café's session gate (script 32:6230) is lifted only for the first meeting; service is still refused. |
| Guest's first café visit | after the player talk "You learned a new piece of music, ???28!" (earlier run "???13") on the guest only; the host (also first visit) got none. The `???` number looks like a title not resolved; not checked solo. |
| Shop: Daisy's (234), host | **works**: talk across the counter, menu Talk / Wonderous flower / Growing berries / Buy berries / Cancel, Buy berries: host money 3000 -> 2900, item 384 in its bag; the guest's console unchanged. |
| Shop: Daisy's, guest | **works**: guest money 3100 -> 3000, item 384 in its bag; the host stays 2900 / one 384. Each console shows its own record. While the host talked, the guest's screen showed Daisy and the host's copy "In event..." (the game's busy bubble). A beside the other student opens the players' talk instead of Daisy (driver spoiled it twice). |
| Dec 25, 21:00 (disc1 saves, `/home/claude/run/xmas4/s/disc1`) | both: g1000 = 40, g1001 = 1, g1002 = 1, g1007 = 10, g1010 = 6, g1011 = 1, g1024 = 0; the host's bag holds the present 123, the guest's none; 8 tree cells (ground id 123) in the town grid on both; **no Santa at Lovers Point** on either screen (`shots/x_lp.png`): block 11 wants g1010 <= 5 or g1024 = 1, and g1024 is 0 after the reload, so a student holding a present cannot ask Santa for anything on the 25th (the game's rule for these saves? not compared solo). |
| Dec 26 (clock 04:27, the day turned at 4:30 in the session) | **works**: both consoles reconnected after "the visit ended; home for the new day"; g1000-g1011 all 0 on both (mystery ended), tree cells 0 on both, **present 123 gone from the host's bag**, nothing left in either (`xs.py`). |

**Seen, not fixed**
- Café service in a session (above): `mq_coop_gate` lifts the flag 212 test at the introduction, but the order branch still
  refuses ("we're not open yet").
- Guest's top screen at Lovers Point (Dec 25 night): rectangles of colour noise where the host's screen has clouds
  (`shots/x_lp.png`, right half); gone on the next load.
- After the guest left the café by the door mat the guest's `free` test stayed false for ~400 frames (loading), no fault seen.
- The town at e 221 on Dec 26 shows the host's console without the guest's copy where both stood at the same point
  (the guest's console shows both); not examined.

**Not covered**: a purchase from a display by touch (Daisy's counter only), Harrows, the café's order (refused).
QUICK level, no regression run. Binary copies deleted.

## The café serves in a session; the guest's "learned a new piece of music" (2026-10-31)

From the net4 playtest's open items. Own folder `/home/claude/run/cafe/` (link `cafe1`), host `day1done.sav` (AAAAAA),
guest `second_day1.sav` (NORA), `--clock 2000-07-18T12:50`, every step by pad.

- **Café service: bug, fixed.** Not the event's t212 gate (`mq_coop_gate` lifts it, but no event command ran at all:
  a log of every command 0x38 during the talk was empty). Two other refusals: (1) Marcy's first line is a talk-start
  record (talk/TStart.dat, main 0x0201A040): 6231 "not open yet" wants t212 in 1..2, 6228/6229 "There's a seat at the
  counter" t212 = 0; (2) the order is A at a stool, map_ss 57 record 29 -> event 217 block 1, kind 4, refused by the
  host's request check. Fixed by `mq_cafe_var` (coop_talk.cpp: t212 reads 0 for actor 131's records) and 217.1 in
  `mq_myst_walkin`'s list (coop_myst.cpp); docs/COOP.md "The café". Same-PC link: host 3000 -> 2800 (guest 3100
  unchanged), then guest 3100 -> 2900 (host unchanged), "Would you like something to drink?" menu
  (`/home/claude/run/cafe/ch/sit_menu.png`). Internet mode (80:40 ms, 1% loss): guest 3100 -> 2900 with the host's
  3000 unchanged, then host 3000 -> 2800 with the guest's 2900 unchanged; host log "scene 21701 (kind 4) asked by
  slot 0 / slot 1: allowed". QUICK PASS.
- **"You learned a new piece of music, ???N"**: the game's own rule, each student's own record. main 0x0202DBC8: when
  the playing song (field +0x15240, set from the BGM id - 0x385 by ov001 0x020CF994) is 1..79 and not yet in the
  student's record, the student's action 29 (main 0x020269C0) learns it and shows 999:2204. The café's song is the
  town's: AAAAAA's café plays song 68 alone (no session) and in the session on both consoles, NORA's town plays 30.
  In the net4 runs and in one internet run here both consoles learned a song (the host too, its first café visit),
  each into its own record (the sync's "own record" line); in the same-PC run neither did. Why internet runs met an
  unlearned song (28, 13) and the others not was not found; not a per-student variable slip, nothing changed.
- **Dec 26, host's console without the guest's copy at e 221**: not `arrival_end` (45da7ca): it only acts on a copy in
  action 7 whose student stands over half a unit away, and only sets the action back to 0, never hides it. Both
  arrived at the same arrival point, so the likely cause is the copy drawn inside the host's own student (the guest's
  console sets the host's copy beside); not run again, not verified.
