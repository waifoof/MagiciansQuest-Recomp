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

## Doors in and out, every walk-in building, same-PC and internet (2026-10-31, report only, no game code changed)

Binary of 61ec19e (tree clean, copied, deleted after), own folder `/home/claude/run/bld/` (link `bld1`, `bt.py` reads each
console's copy of the other student from RAM: field +0x3E4C, x/z +0x2C/+0x34, action +0xA2, plus the kept bytes at
session +0x49E8 + slot * 0x50 + 0x20; `bdoor.py <building>`), host AAAAAA, guest NORA, by pad: warp to the arrival point,
walk to the door, Up, A; the host in, the guest in, the host out, the guest out; samples right after the load and 1 s and 2 s
later. Same-PC link at 12:50, internet (`NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`) at 12:50 and 14:05.
Buildings reached: dorm (221), school (240), café (235), Harrows (233), shop at 237 (area 60, a hairdresser), shop at 238
(area 62), bookshop (243). Not walk-ins: Daisy's flower shop (234, the counter outside) and 236, 239 (Up does nothing),
241/242 ("The door is locked"). The Gateway was not reached.

| Building | Same-PC | Internet |
|---|---|---|
| dorm, school, bookshop | in and out fine; copy right within 1 s | same; the copy was 0.6-1.0 off for the first sample (action 14 or 0), 0.00 at +1 s |
| café, Harrows, shop 237, shop 238 | the second student in: the first's copy in action 7, 1.0-2.0 off, and the shop's greeting open | **stuck for as long as the student stands still** (sampled up to 3 s, and until the first step, 10-20 s): copy of the guest at the host 1.7-2.0 off, copy of the host at the guest 1.07 off, both action 7. After the first step (`free`) 0.00 |
| all, out | fine, 0.00 within 1 s | fine, 0.00 |

Cause of the stuck copy (read from the kept bytes): they say 0x11 with +3 = 7 (the walk through a door), not the standing
0x0F, at the entry point (café (5.0, 7.5), Harrows (9.5, 26.5), 237 (4.0, 8.5), 238 (5.0, 8.5)), so `arrival_end`
(`stands_elsewhere` wants 0x0F and +3 = 0) never acts. The doors where the student walks on at once (dorm, school, books)
are fine. Seen on the same-PC link too (café, shop 237: 2.00 and 1.03 off, texts open). A student reading the greeting
(Marcy, Harrows) stands there for seconds with the partner's copy 1-2 units away: not a break, but not "within 0.5".
Also: a warp (`e N`) made the copy of the other sit in action 7 at the first sample (1.4-3.1 off, internet only).

**Dec 26 e 221 (host without the guest's copy)**: not repeated with the saves (xmas4 not run). Both students at the same
entry point (shop 237, 238, café) show the copy present in the list but in action 7 at 1.7-2.0 units; the shot of two
close students (`/home/claude/run/bld/shots_same_stuck.png`) shows both on both screens. So "same point" alone does not hide a copy; a copy stuck in action 7 off its student is the likelier reading.

**Seen, not fixed**: in the first internet pair the host's game (not the runtime) stopped for good at 13:00 on the clock
(frames 37026, "wireless: instance 0 sent nothing for 4075 ms", then no frame end: no MQ_POS_FILE, warp or RAM dump
answered, gdb: the loop fn_main_0200cb08 -> 0200c928 -> ntr_poll under ov001 020c57a0) just after the host left the school
with the guest inside; machine load 5-7 (another agent's pair). Not reproduced in the second pair (14:05). Saved log:
`/home/claude/run/bld/inet/host_frozen_log.txt`.
QUICK level, no regression run. Binary copies deleted.

## Battles inside mysteries: the Gremlin (mystery 17) by real input in a pair (2026-10-31, fix in docs/COOP.md "Battles inside mysteries")

Pairs of `day1done.sav` (AAAAAA, host) and `second_day1.sav` (NORA, guest), same-PC link `btl1`, Connect now, own
binary copy, `var 411=0` on both (harness `/home/claude/run/battle/`: `pair.sh`, `lib.py`, `h2.py` host leads,
`g3.py` guest leads, `fight.py`). Decoded first: only event 150 fights (query 0x67); the battle mysteries are 5, 18,
46, 49 (Captain Dot, played since: see "Captain Dot's battle" below), 34 (the Nightmare) and 17 (the Gremlin, cheapest).

**How the Gremlin goes (mystery 17, a rotation mystery), for players.**
1. Stage 0: visit the school's organ room (map 7, the Organ Man) by day; a Gremlin is fiddling with the organ.
2. Stage 1: come into the organ room exactly on the hour (xx:00, 8:00-19:00) disguised as an object (s98 = 46);
   otherwise you only hear "*rustle rustle*" or it runs off.
3. Talk to the Organ Man (A at the organ): "Huh?! Where'd you come from?!", then the battle: when the Gremlin stops
   moving, press X to hit him; three hearts each, be quick.
4. Won: he promises to stop (stage 3, a reward); lost: "Not bad, not bad!", back to stage 1.
Together: the other student is asked "Go and help?"; Yes takes it into the organ room and the same scene; each fights
its own Gremlin and gets its own result; the host's result decides the stage.

| What | Result |
|---|---|
| The host's battle, not joined (before the fix: the organ talk was not told) | **works** alone in a session: battle HUD (`hb2_3.png`), lost by time-out, 087.4 "Not bad", stage 1; the guest was asked "Go and help?" for the *result* scene (fixed: not told now). |
| Host leads, guest joins (fixed binary) | **works**: asked, Yes, joined in map 7 (`j1.png`), both battles, both results, stage 1 on both. |
| Guest leads, host joins from town (fixed binary) | **works**: guest's stage write taken at once, both battles, the guest's result waited for the host's, stage 1 on both (`g3_end.png`). |
| Before the fix, guest leads | the joined host's 087.2 would have reset the hiding; the guest's 221.12 result applied after the battles undid the host's joined copy (stage 1 -> 2 -> 1, seen). |

Not reached: a win (pressing X at random never hit), Captain Dot, the Nightmare, the walk-in 087.3. The hiding
(s98 = 46 or g1010 = 1) and the mystery itself were poked. Harness note: the "Go and help?" Yes must come ~40 frames
after the question's first line, or the press is lost. QUICK PASS. Binary copies deleted.

## Shops' entries: the copy of a student held at the entry; the host's hang leaving the school (2026-10-31)

Own folder `/home/claude/run/door2/` (link `door2`, `bdoor.py` from the buildings check with the local student's action
added, `seq.sh`, `samepair.sh` for the same-PC link), host AAAAAA, guest NORA, 12:50, by pad; internet mode
(`NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`) and the same-PC link, under load 6-7 (another agent's pair).

**What 0x11 with +3 7 is.** The student itself is still in its own walk through the door: at the café, Harrows and the
shops at 237/238 the student stands at the entry point in action 7 (read from its console: `a7`) while the keeper's
greeting is open ("Oh? We haven't met", "How do you do?", "Hello there"), and walks on (about 1.2 units) only once the
text is closed. Its console sends 0x11/7 with the place during the walk, then nothing new until it walks on. The
copy never got there: on the host the guest's copy stood at the door where its first 0x11 put it (1.7-2.0 off); on the
guest the host's copy, whose spot the guest's own student (also held at the same entry point) stood on, kept walking
round it in 7, about 1.07 off (`MQ_ARRIVAL_LOG=1`). The dorm, school and bookshop have no greeting: the student walks
on at once.

**Fix** (`arrival_end`, coop_talk.cpp): kept bytes 0x11/7 of this area at one place for 1 s, over half a unit from a
copy in 7: the copy is set back to 0, put at that place with its walk's goal (+0x38, main 0x020325BC) there, or 1.19
units beside it on the copy's side when the local student stands on that spot (put on the spot, or at 1.0, the
student's push held it ~1.09 off, walking on the place). Results, the second student in, samples at +1 s and +2 s:

| Building | Internet (both consoles) | Same-PC (both) |
|---|---|---|
| café, Harrows, 237, 238 | before: 1.7-2.0 / 1.07 off in 7 until the first step; after: 1.19 beside, from +1 s | 1.19 beside from the first sample |
| school | 0.00 at +1 s (not touched) | - |
| all, after the first step | 0.00 | 0.00 |

Log lines "coop talk: slot N's copy walked through the door while its student is held at the entry (x, z): put at
(x, z)" on both consoles, all four buildings, both links. The copy shows action 1 there while standing still (its place
does not change between samples); during the greeting the camera is on the keeper (`/home/claude/run/door2/
shot_shop238_held.png`), so neither student is in view. Both students on one entry point is the game's own: they cannot
be closer than ~1.1. QUICK PASS (door2q).

**The host's hang (seen in /home/claude/run/bld/inet, reproduced here in 1 of 3 internet pairs that ran the school step, 2 of 5 in all: the
host out of the school, then the guest out; game clock 13:02-13:03; the 13:00 bell is not involved).** A real hang in
the game's code, not the harness: the frame counter runs on, but the game's main loop never ends a frame (warp, pos and
RAM dump requests unanswered). gdb (attached once per read, `/home/claude/run/door2/inet6/host_frozen_log.txt` the log):
the game thread spins in the display-list scan of main 0x0200C928 (the loop 0x0200C9CA-0x0200CA3E, its end r12 =
0x0F0022E1, the data pointer 0) under main 0x0200CB08 <- 0x02034D2C <- ov001 0x020C57A0 <- main 0x02030CB8, the
remote players' loader thread (state +0x630 0: making a copy's model). Its player list (0x020B8AE8) entry 0 is the
guest's copy (slot 1) in state 0 with **no model (+0x20 = 0)**, map 0xFE; 0x020C57A0 is given that model 0 and reads
its +0x37C/+0x380 from low memory (r4 = 0xEB000032, ARM code bytes), so the scan's bounds are garbage. The same
"copy without a model" as the 0x1D ground packet's crash (coop_talk.cpp `mq_coop_remote`). Not fixed: why the copy
reaches the loader's state 0 without a model (an allocation failing, or its model freed by the host's field change
while the guest was still in the school) is open; a guard needs a hook on ov001 0x020C57A0 or main 0x02030CB8
(regeneration). Binary copies deleted.

## The host's hang after leaving the school: found and fixed (2026-10-31)

Own folder `/home/claude/run/freeze/` (link `frz1`, the door2 harness copied: `soakpair.sh`, `loop.py` the school step
in a loop, the host's game checked by a RAM dump after each), host AAAAAA, guest NORA, internet mode
(`NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`), 12:50, `MQ_LOADER_LOG=1`, own binary from a private tree (others' edits left
out), load 5-8.

**Reproduced.** The loop as bdoor.py (both in, host out, guest out at once): 8 of 8 fine. With the guest left in the
school 1500 frames after the host went out (as bdoor.py's slow checks did): hung at rep 7 (guard off), the log
(`/home/claude/run/freeze/hang_guard_off_host_log.txt`): `coop loader: model 1 taken from player object 0 (slot 1,
state 3) by 00000000 (lr 02100f47)` about 1000 frames after the host's town came up, then at the guest's arrival
`player object 0 (slot 1, map 254, state 0) is made without a model ... (guard off)` and no frame end again.

**Cause.** lr 0x02100F47 is the game's own town model manager (ov002 0x02100E30), which our `mq_town_models`
replaces in a session, except when the field's session byte reads 0. It reads 0 inside the people's calls coop_npc.cpp
runs as in single player (`call_solo`); the town field's update (ov002 0x02101538, the call at 0x02101580) is reached
from inside them, and the game's manager, as alone, gave the absent student's model back (bind (model, 0) clears the
holder's +0x20). When the guest came in, its copy's look was remade (state 3 -> 0) with model 0: ov001 0x020C57A0 read
the model's +0x37C/+0x380 from low memory.

**Fix** (docs/COOP.md "Inside our calls run as alone"): `mq_town_models` goes by the session's byte (the saved one
inside `call_solo`); plus a guard in the loader (ov001 0x020C57A0: the copy's own model bound back first). After, guard
OFF so only the cause's fix acts, the same loop with the 1500-frame wait: **8 of 8 reps, no hang**, never a model
taken from player object 0 (only the empty slots' 2 and 3 by the game's request task, as before), "handed out as in the
session inside a call run as alone (lr 02101585)" 5 times (each one a moment that took the model before). Afterwards
both students drawn on both consoles in front of the school (`/home/claude/run/freeze/final_pair.png`); copies 0.00
off. QUICK PASS (frz1). Binary copies deleted.

## Captain Dot's battle (mystery 5) and the island's air taxi (2026-10-31)

Pairs of `day1done.sav` (AAAAAA, host) and `second_day1.sav` (NORA, guest), same-PC link `dot1`, Connect now, own
binary copy (`/home/claude/run/dot/`), `--clock 2000-06-06T21:00`, every step below by pad; prerequisites poked:
`var 411=0`, `var 914=0`, g1000 = 5, g1001 = 3, g1244 = 0 on the host in town, money. Docs: COOP.md, "Captain Dot's
battles".

| What | Before | Now |
|---|---|---|
| Lovers Point, onto the taxi stop's mat (selector 247) | nothing (222.1 is kind 4: refused in a session) | the driver lands, the flight (map 65), the island (map 67); host and guest |
| The island's stop (selector 249) | (not reached) | back to Lovers Point (maps 56, 49); host and guest |
| The café at mystery 5 stage 3 | Captain Dot placed without a model: nothing to talk to | drawn; A: "Hey there, you little sea rat!" |
| Host talks to him, pays, answers (menus shuffled: read from shots) | - | the guest asked "Go and help AAAAAA?", Yes, joined in the café, answered its own copy; both battled (map 70), both lost, both result scenes on the island, home, stage 5 on both |
| Guest talks to him | - | the host told "NORA has found something mysterious!" (its Yes too late: "Looks like..."); the guest's battle and result; the host took its stage (5) |

Not reached: a win (X at random loses), mysteries 18, 46, 49. QUICK PASS. Binary copies deleted.


## Soak 4: internet mode at 200 ms, every building one after the other, the school exit, a day turn (2026-10-31, 1d55feb)

Binary of origin/main 1d55feb (copied from the verified build), `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`, link `soak4`, host
`day1done.sav` (AAAAAA), guest `second_day1.sav` (NORA); driver `/home/claude/run/soak4/soak4.py` (soak3's, steps
`buildings`, `schoolhang`, `townwalk`, per-second copy samples `copies ...`). Three segments, about 85 minutes, 156
state comparisons: **0 fatal lines, 0 stalls over 3 s, 0 frame-rate drops, town block / clock / date / `.coop` equal at
every check in the session**. Resident memory flat: main 341.8 -> 343.3 MB over 50 min (segments 2 and 3: 341.9
unchanged), stand-ins 349.6-350.0 MB unchanged. No black screen in the shots read.

| Segment | What | Result |
|---|---|---|
| 1 (Jul 18 14:00, 50 min) | town together/apart, a chat and a gift each way, dorm, school, café (+ drinks), Harrows, bookshop, 237, 238 each: host in, guest in, host out, guest out; the school exit with the guest staying 1500 frames longer (2 reps); a walk; homeroom and a lesson; host Disconnect (both "Save complete.") | no divergence |
| 2 (Jul 19 02:55, from those saves) | connected, the day turned in the session at 3:00, both connected again ~25 s later (copies absent for the first sample only), a walk, Disconnect and save | no divergence |
| 3 (Jul 19 10:00, from those saves) | the seven buildings again (patched driver), the school exit with the guest 1500 frames longer (2 reps), Disconnect and save | 1 flag, see below |

**The old hang did not recur**: 4 of 4 reps (host out of the school, 1500 frames, then the guest out), frame counters on
both consoles advancing, copies 0.00 off afterwards (`shots/soak4/d1_h0_out.png`, `d3_h1_out.png`).

**Copies** (one sample per second after the second student's entry, 5 samples): dorm, school, bookshop, Harrows,
237, 238, café: from +2 or +3 s 0.00-0.07 off on both consoles, never over 1.3 after +3 s; during the first second
or two the other's copy is absent (115, 0) or at the door (0.6 off in the school). When the first student leaves, the
other console shows no copy (parked at 115, 0): as intended.

**Seen, not understood or not a fault**
- *Café, segment 1* (`cafe_2in`, the keeper's first greeting "Oh? We haven't met" open on the host, guest just in):
  the host's console showed the guest's copy at (5.0, 9.5) in action 7, **2.0 off** the guest's real (5.0, 7.5), about
  10 s after the guest's A; the guest's console had the host's copy 1.2 off (fine). That is the pre-fix symptom of
  "Shops' entries" above; not flagged (both students in action 7), and a second visit (segment 3, no greeting) was
  0.00 from +3 s. The 1 s hold of `arrival_end` may not be reached under 1% loss; no per-second samples exist for
  the greeting case, worth one targeted repeat.
- *Segment 3, `harrows_2in` flag "its copy on the other console is not here"*: the host's copy WAS there (0.06 off,
  model present); only the object's `here` flag (+0x5A0) read 0 for the 4 s recheck. Same false positive as soak 3.
- *Host's café order*: texts as the guest's order (menu, "It costs", "Today's special", "Hohoho"), but money stayed
  3000 (segments 1 and 3 both; the guest's order took 3100 -> 2900 in segment 1). Not told apart from the driver (the
  menu answers are the driver's A presses); check by hand. The guest's order in segment 3 became a talk with the host
  (stood by the stool): driver.
- *Driver*: a box "You learned a new piece of music" (each student, once) holds the student; the first version of the
  step only pressed Down and the guest stayed in the café ~5 minutes until the warp. Fixed in the driver (B presses).
  Transient `pos` 3.2 off at `books_2in` and classmate-need bytes in `town` rechecked equal 4 s later. The walking
  routes stuck at (39,58)/(50,37) for minutes again (soak 3's note).

**Not covered**: shop purchase, fishing/insects, a classmate talk on the second day. Binary copies deleted.

## Winning a mystery's battle: how it is played (solo win) and why a session can't win it yet (2026-10-31)

Own folder `/home/claude/run/win/` (link `win1`; `solo.sh`, `w1.py` the Gremlin's battle by real input alone: the
hiding at 11:00 and the organ talk, `w8.py`/`wlib.fight` the fight; `pair.sh`, `p1.py`/`p2.py` host leads, guest
joins; `fight_*.log`), saves AAAAAA (host, solo) and NORA (guest), binaries from this tree with the research switch
`MQ_BATTLE_LOG=1` (cfie_hooks.cpp, `mq_frame_end`: the battle object's hearts, state, pad, places as they change).

**How to win a battle (players' controls).** The battle object (overlay 19/20/21 at 0x0210A400, object
`*0x0210A3E4`; hearts +0x20 the student, +0x21 the monster; state +0x22) cycles: state 1 the monster hops about (8-15
hops), state 2 it runs to a spot and stops (flag 245 = 2), state 4 it stands there about 60 frames, then state 1
again. **X casts a magic shot** (spell kind 7, the student's action 4) that flies to the monster by itself (no aiming,
no D-pad, no touch screen). The shot only hurts the monster if it lands while the monster **stands still** (main
0x020448EC: actors 171 / 197 / 214 take kind 7 only while flag 245 = 2); a shot that lands while it moves makes it
run off and start over (state 0). So: **wait until the Gremlin stops and faces you, press X once, then wait** (the
monster stays put until the magic lands, about 4-5 s with the hit's effects; more presses meanwhile only waste time:
with X every 10 frames one battle took a single heart). Three hits win; 45 s (2700 frames, the object's +0xC) without them lose ("Not bad, not bad!"). The Gremlin
never took the student's hearts in these battles. Spamming X (the earlier harnesses) loses: each cast that
lands while it moves resets it.

| What | Result |
|---|---|
| Solo (AAAAAA alone, real input: hiding at 11:00, organ talk, X once per stop) | **won** twice: hits at t 491, 989, 1469 (`fight_win1s.log`), "Tsk! Guess you're not a total..." (087.4 won), **stage 3, g1070 = 2**, g1010 = 0 (`/home/claude/run/win/solo_end.png`). |
| Pair, host leads, guest joins, both press X once per stop (same-PC link, before any change) | **both lost** by time-out, hearts 3/3 on both, stage 1 on both, no crash (`p1_before.out`). |
| Pair, host alone in its battle (guest declined) | lost the same way. |

**Why a session can't win (found, not fixed).** (1) The shot's step (main 0x02036184, a method of the shot object)
skips the target check and the hit (0x020448EC, 0x02021768) when `*(*0x020ACA58 + 0xA28)` (the session) is set: the
shot just ends at the person it reached. (2) Both students are put on the same spot by the battle's script (actor_set_pos
player 4.5, 7.0) on both consoles, so each shot reaches the other student's copy first (the person finder, ov001
0x020C3704, takes the first object in reach with a model; the caster is left out). (3) Past those two (tried: an
`instead` hook on 0x02036184 that runs it as alone during a battle, flag 241 & 3 = 2, and hides the other students'
models from the finder; the shot then reached the Gremlin, check 1), the heart still was not taken: the hit's effect
goes on in a session through a request to the host (kind 0x44, `44 00 64 01 .. 07`, then event 382.4, an empty block)
instead of the solo path (no script: the heart is taken by native code, query 0x6D's handler ov028 0x02117BEC ->
the battle's vtable +0x30, reached from a magic effect thread, main 0x02036D58 started at the hit). The attempt is kept
as a patch (`/home/claude/run/win/full_attempt.patch`, its coop_myst.cpp/hooks.txt part) and not committed. Next: find
where the hit's damage is decided in a session (the 0x44 request's sender, and the thread 0x02036D58's session
tests), and run that as alone for the battle's monster.

Also seen: after the host's battle that the guest declined ("Go and help?" No), the guest's copy of the town kept
g1001 = 2, g1010 = 1 (the host's: 1, 0; "coop sync: this console wrote g1001=2: the host's alone, not sent" right
after the offer's question, never put back), so the next organ talk was not offered to it. Not looked into.
Not reached: Captain Dot's win, the island's ride home after a win. QUICK PASS (the research switch only). Binary
copies deleted.

## Soak 4's two café leads: Marcy's first greeting, the host's order (2026-10-31, fix in docs/COOP_INTERNET.md "The newest movement packet is sent again")

Own folder `/home/claude/run/cafe2/` (link `cafe2`, the café harness copied: `soakpair.sh` / `samepair.sh`, `t1.py`
entry and orders with per-second copy samples and money, `t5.py` repeated entries), host `day1done.sav` (AAAAAA),
guest `second_day1.sav` (NORA), neither has met Marcy, `--clock 2000-07-18T14:00`, every step by pad, internet mode
(`NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`) unless said; binaries built from a private copy of the clean tree.

- **Lead 1, the copy 2.0 off during "Oh? We haven't met"**: by pad in 4 internet pairs and 1 same-PC pair (host in,
  greeting held, guest in): `arrival_end` acted every time on both consoles ("walked through the door while its
  student is held at the entry ... put at", `MQ_ARRIVAL_LOG=1`: kept 0x11/+3 7 at the entry point (5.0, 7.5), the
  copy at the door (5.0, 9.5), held 60 -> put), 1.19 beside from +2 or +3 s, also with the greeting open on both. So
  in soak 4 the kept bytes themselves must have said the door: the walk's last 0x11 (the entry point) is a single
  movement packet, newest-wins and never resent, and the student sends nothing more while held. **Fix**: the newest
  movement packet goes again 60, 200 and 500 ms after (COOP_INTERNET.md). At 20 % loss (to make the loss common),
  both in one after the other six times: before, the guest's copy of the host 1.0-1.1 off or absent at +4 s in 3 of
  6; after, 0.00 in 6 of 6 and every copy on both consoles 0.00 at +8 s (12 of 12); `repeats out` in the log.
- **Lead 2, the host's order**: the host pays and drinks. Money by pad (3000 for AAAAAA): 3000 -> 2800 and then
  200 per order in 9 of 9 host orders over 5 internet pairs (one with `MQ_SCRIPT_TRACE`, one under gdb, three
  without either, old and new binary), the guest 3100 -> 2900 in its orders, the other console unchanged each time;
  the host then sits at the stool (4.0, 4.5). Same-PC pair (new binary): host 3000 -> 2800, guest unchanged. Where the money goes: not the event's query
  0x3B (ov028 0x021160C4 only tests that 200 is affordable, into flag byte 100) but the text: a box's money change
  (box +0x109C) applied as its text runs (main 0x02014494 -> 0x020144F0 -> 0x02017744 -> ov001 0x020C8A6C, record
  = (box +0x17F0) +0x18: the field's own student; gdb, both consoles: amount -200, the field object, record
  0x020AD824). Nothing of ours touches those box fields. The two misses (soak 4 segment 3; my first pair here, the
  same texts, no "+6a0" record sync) did not come again in the same sequence (closed greetings with B, the order at
  the middle stool) with either binary: not found. (Soak 4 segment 1's host "order" was a talk with NORA: driver.)
- Driver notes: after a drink the student stays on the stool; a second order by the other student then becomes a
  player talk unless the first walks off (Down, Right).

QUICK PASS (cafe2q). Binary copies deleted.

## Winning a mystery's battle in a session: the Gremlin by real input at 200 ms (2026-11-01)

Own folder `/home/claude/run/win2/` (`pair2.sh`: internet mode `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`, link `win2`,
host AAAAAA `day1done.sav`, guest NORA `second_day1.sav`, `MQ_BATTLE_LOG=1`; `p3.py` host leads: the hiding by walking
into map 7 at 11:00 disguised (s98 = 46 poked), the organ talk, the guest asked "Go and help?"; `g4.py` guest leads
(the hiding poked on the host, as battle/g3.py); both fight by `wlib.fight`: X once each time the Gremlin stops).
What was wrong and the fix: docs/COOP.md "Winning a battle in a session" (traced alone with gdb watchpoints: the hit,
the student's reaction step marking the monster's director slot, the director's record, the reaction script's query
0x6D). Each console's battle stays its own: its hearts, result and reward; the host's result decides the stage.

| Run | Result |
|---|---|
| Host leads, guest declines (R3) | **host won**: 3 hits (t 1344), "Tsk! Guess you're not a total...", stage 3, g1070 = 2, g1010 = 0 on **both** consoles (the guest's town the host's). |
| Guest leads, host declines (G2) | **guest won**: 3 hits (t 1506), its result scene's writes "to the host: g1001=3 g1009=17 g1010=0 g1070=2", "the host took our scene's stage": stage 3, g1070 = 2 on both. |
| Host leads, guest joins, both fight (R7, R8, R9, R11 final binary) | **host won** each time (3 hits, t 1350-1428; "Tsk! Guess you're not a total loser.", `/home/claude/run/win2/res_w2h_R11.png`), stage 3 / g1070 = 2 on both; **the guest's own battle lost** each time: its shots never reached its Gremlin (the monster never waited for a shot, as it does alone), "Not bad, not bad!" with the host's copy shown again beside it (`res_w2g_R11.png`). |
| Loss: host leads, no X, guest declines (L1, final binary) | lost at t 2701, "Heh heh heh! Not bad, not bad!" (`/home/claude/run/win2/res_w2h_L1.png`), stage 1, g1010 = 0 on both. |
| Before (same harness, binary of e4410f8 + only the shot's hook) | every battle lost, no heart taken (the hits "reached actor 197 (check 1)", the monster's record never matched). |

Found on the way: (1) with a joined student both students were put on the battle's spot and the copy pushed this
student 1.2 units aside; from there no shot reached (only the console whose student stayed at (4.5, 7.0) hit): the
copies are hidden for the battle and the student kept on its spot. (2) In the joined runs before the fix both
battles stalled at once mid-battle (the Gremlin still in a hop until the time ran out): the cast's scene 382.4 asked of
the host on both consoles; gone with the cast as alone. (3) The earlier "guest kept g1001 = 2 after declining" is
the guest's "Go and help?" question still open (the harness's No had been lost): the town's stage is held while an
event runs there; answered No, "the host's values back (no event runs): g1001=1 g1010=0" at once. Not a fault.

QUICK PASS (final binary). Not proven: a joined guest's win in its own battle (its shots do not connect while the host also fights; alone the
guest wins), Captain Dot's and the Nightmare's battles.

## A joined pair's battle: both students' hits count (2026-11-01, follow-up)

Folder `/home/claude/run/win3/` (as win2, link `win3`, `MQ_BATTLE_SHOT_TRACE=1`: our shot's place and direction each
4th step). Why a joined guest never hit: its shots flew south. A shot flies along its caster's facing (+0x68); the
guest's student, pushed off the spot by the host's copy in the battle's opening, faced 180 (the host's 90; alone the
cast turns the student to the monster, 67.5 for the Gremlin at (7, 8)): `dir (0, 0, -0.063)`, past the Gremlin. Fix
(`battle_spot_frame`): while the student casts (action 4) it faces the monster. Hiding the copies' models for the whole
battle (in 07e0daf) hung a host ("player object 0 ... made without a model; model 1 held by" the hidden copy): removed.
All at 80:40 / 1% loss, host leads, guest joins, both press X once per stop:

| Run | Result |
|---|---|
| J6, J7 | **shared win**: both 3/0 (t 1450-1896 host, 1627 guest), both "Tsk! Guess you're not a total..." (`res_w3h_J7.png`, `res_w3g_J7.png`), stage 3 / g1070 = 2 / g1010 = 0 on both |
| J4, J5 (facing first held at 90) | the guest won, the host lost (its Gremlin stopped at (5, 5), shots flew +x): why the facing follows the monster |
| L2 (no X on either) | both lost at t 2701, "Not bad, not bad!" on both, stage 1 / g1010 = 0 on both |

QUICK PASS. Not tried: Captain Dot's and the Nightmare's battles joined.

## Battles' rewards and Captain Dot joined, at 200 ms (2026-11-01, fixes in docs/COOP.md "After a battle")

Folder `/home/claude/run/battle2/` (link `battle2`, internet mode `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`, host AAAAAA,
guest NORA; `pair2.sh`, `p3.py` the Gremlin as win3; `dprep2.py` Captain Dot's set-up (pokes for prerequisites only:
`var 411=0`, `var 914=0`, g1000 = 5, g1001 = 3, g1244 = 0, 5000 Ritch), `yes.py` the guest's "Go and help?",
`dq2.py`/`step2.sh` the talk by pad, each shuffled menu read from the pair's screenshot, `fd.py` the fight by the battle
log and the result scene; `dm.py`, `vars.py` the done mark and variables). Binary of 416e346, then with the fixes.

| Run | Result |
|---|---|
| Gremlin solo (AAAAAA) | won: bag unchanged (item 1468 only), 3000 Ritch unchanged, "AAAAAA resolved a mystery!", stage 3, done mark 17 set |
| Gremlin pair, host leads, guest joins (J1) | both won (t 1402 / 1627), "AAAAAA ..." / "NORA resolved a mystery!", bags and money unchanged on both (as solo), stage 3, g1070 = 2, done mark set on both |
| Captain Dot, host leads, guest joins, both win (D1, 416e346) | both won; both bags + item 392, 5000 -> 4000 Ritch, g1001 = 7, g1575 = 5 on both, both on the island; **the host's copy on the guest wore the wizard's hat and staff** (`isl2.png`, `isl3.png`) |
| same (D3, hat fix) | the host's copy of NORA fixed ("slot 1's copy kept the wizard form ...: its look taken again"); **the guest won but got no reward**: its battle started at stage 3 (the host's value taken back after 075.4), no 075.6, stage held at 3 |
| same (D4, both fixes) | both won, item 392 + 1000 Ritch paid on both, stage 7 / g1575 = 5 on both; the guest's copy of AAAAAA fixed (`isl_D4.png`) |
| loss, guest answers first, host 10 s later (D5) | the race again: "the battle starts at the stage our scene wrote: g1001 3 -> 4"; both lost by hearts (0/3), both "Had enough, have we?", both sent to their rooms, stage 5 on both, bags unchanged, 1000 Ritch paid on both |

Seen, not followed: in D3/D4 the guest's screens showed the talk on the top screen with no 3D (until the battle); D1/D5
normal. The Nightmare (mystery 34) not played: 104.1 sets the stage to the starter's room (g1001 1-4 = rooms 22-25),
the night walk-in there (104.2, selectors 36-39) leads into the dream map 40, whose walk-in 40 (104.4) wants item 429
in hand; the result scene 104.5 plays from the room loads by that stage, so a joined student in another room likely
finds none; not reached in the time. QUICK PASS. Binary copies deleted.

## The Nightmare (mystery 34) joined, at 200 ms (2026-11-01, fixes in docs/COOP.md "After a battle")

Folder `/home/claude/run/night/` (link `night1`, internet mode `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`, host AAAAAA,
guest NORA, clock 21:00; `pair.sh`, `prep.py` (pokes for prerequisites only: `var 411=0`, `var 914=0`, g1000 = 34,
g1001 = 0, g1010-1012 = 0, g1036 = 0, g1244 = 0, item 429 in bag slot 1 on both), `run.py <tag> <host> <guest>`,
`fyes.py`, `nfight.py`, `solo.sh`/`solo2.py`). The dorm's load gave stage 1 (s1 = 22 on both saves). The test saves'
rooms have no bed, so the room's walk-in (action 44, lying down) was not reachable: 104.2 queued with `MQ_EVENT_FILE`;
from there all by pad: the dream's walk-in by walking, the guest's Yes, the battles, the result scenes.

| Run | Result |
|---|---|
| Host alone in a session (W1, first binary) | lost (1 heart: the Nightmare's magic), 104.3 "Hehehe... You're a fool", room 22, 104.5, stage 1, g1010 = g1011 = 0 |
| Joined, both lose (L1, hold fix only) | **the guest's join was taken by the dream's load scene 104.3**: no 104.4, no battle on the guest; the host's battle restarted every ~3000 frames with the guest's copy standing in the dream, its result held past the 10-minute limit |
| Joined, both lose (L2, both fixes) | guest: "scene 10404 queued (joining)", both battles, both lost, both 104.3, both 104.5 in room 22 ("the stage held until our room's scene"), stage 1 / g1010 = g1011 = 0 on both, bags and money unchanged |

Not proven: a win. Alone too (solo, no session) X once when the Nightmare stops (state 2.0 at (5, 7)) never hit: its
state goes 2.1-2.3 then 5 (its own magic; a cast in progress is not hit, otherwise the student loses its one heart);
turning toward it (Down) first did not hit either. In the session the shot trace showed our shots flying -z (the
student's facing 180, set by 150.5) while the monster stood +z: how this battle is won is not worked out. Not tried:
the guest starting (a guest cannot use room 22). Seen, not followed: the guest's camera in room 22 after its 104.5
looked from the room's corner (`end_ntg_L2.png`). The Captain Dot "talk on the top screen, no 3D" on the guest was not
reproduced here (not looked at in a Captain Dot run). QUICK PASS (L2's binary). Binary copies deleted.

## The Nightmare won, alone and joined at 200 ms (2026-11-01, docs/COOP.md "Winning the Nightmare")

Folder `/home/claude/run/night2/` (own tree copy and binary, link `night2`; `solo.sh`/`s1.py`/`s2.py` alone,
`pair.sh` internet mode `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`, `prep.py` (pokes for prerequisites only, as night/),
`run2.py <tag> win|lose win|lose`; `nf3.py` the fight: when the Nightmare stops (state 2.0) the D-pad toward it for 8
frames (the camera's 22.5 degrees counted), X `XGAP` frames later; `ana.py` facing / wanted angle / shot direction).
Host AAAAAA, guest NORA, 21:00; the dream started with 104.2 queued (no bed in the test saves), then all by pad.

**How to win (players).** When the Nightmare stops moving, turn to face it with the D-pad (on screen; the dream's
camera is turned), wait a moment until it starts its own spell (about half a second after it stops), then press X
once. The shot flies straight the way you face; it only hurts while the Nightmare is casting its own spell. Casting
also shields you from its magic (you have one heart; standing still without casting, its spell takes it). Three
hits win.

| Run | Result |
|---|---|
| Alone, X at the stop, no turn (A, B, H) | never hit in 10000+ frames: every shot flew north (facing 180) while it stood south; its magic hit the student only once the cast had ended (`fight_D.log`: lost) |
| Alone, turned with the D-pad mapped to the map's axes (T4, T6) | 1 hit in 400 tries: the camera's turn not counted, shots 45-110 degrees off |
| Alone, turned with the camera's 22.5 counted, X 2 frames after (T7) | **won**: 3 hits, t 2105, "Okay okay! I give up! You win!", stage 9, g1054 = 5, g1070 = 2 |
| Joined, both turn + X 2 frames after (W2, before the fixes) | both won (t 4253 / 5808), both results; the guest's 104.5 in room 22 at (0, 0) outside the floor, camera in the wall (`end_n2g_W2.png`) |
| Same (W4) | host won; **guest never won**: shaking between its spot and 1.2 units off (the host's copy on the spot), one heart in 4000 frames, the day turned |
| Copy aside (W5) | both won (guest t 22918): its shots reached 2-5 frames before flag 245 = 2 (X at stop+14) |
| Copy aside, X 14 frames later (W6) | **both won** with 3 presses each (t 1769 / 1738) |
| Final binary (W8) | **shared win**: both 3/0 (t 2183 / 1744), both "Owww... You win!" and 104.5 ("AAAAAA ..." / "NORA resolved a mystery!"), stage 9 / g1054 = 5 / g1070 = 2 / done mark 34 on both, bags and money unchanged (no item reward), the guest put by the door ("our student at (0.00, 0.00) ...") and on the floor in its picture (`end_n2g_W8.png`) |

Not changed: `battle_spot_frame` still turns a casting student in a session toward the monster (a help alone does not
have). The top screen showed the checker backdrop in these pictures (on-demand drawing). QUICK PASS (n2q). Binary
copies deleted.

## Soak 5's café exit: the guest left on the exit spot (2026-11-01, fix in game/src/coop_myst.cpp `own_scene_again`)

Real, not the harness. Own folder `/home/claude/run/cafe3/` (link `cafe3`, `pair.sh` = soak 5's internet pair at
`NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1` with `MQ_MYST_ARB_LOG=1`; `cafe3.py` café trials by pad, `stale.py`, `recs.py`),
host AAAAAA, guest NORA, binary of 9c74676 (copied, deleted after). Doors in and out are scenes the host arbitrates
(kind 3: the café's door 253.37, its exit 255.44). A student waiting for the answer sends its request again every 5
frames; at 200 ms two or three copies are on the way. When the first is allowed (the other student's scene just
ended) the host records it as the asker's scene (session +0x4A28 + slot * 0x50: file, +10 block), and 0x020F0EDC
counts the asker's own record, so the copies behind it get "no" (50); with the jitter a "no" can arrive first, the
student drops the scene and asks for ever while the host's record stays. Seen by pad (old binary): the guest held at
the café's door in action 7 for 8 minutes (2 of 5 trials), the host's record of slot 1 `fd 00 .. 25`, the guest's own
`ff ff`; from then on the host's own door requests were refused too. With that record set to 255.44 by hand
(`stale.py`) the guest walked Down onto the exit spot (5.0, 12.43) and stayed, the host as well: soak 5's picture.
**Fix**: a request for the scene the asker's own record already holds is answered yes ("asked again by slot N").
After: the hand-set exit record let the guest out at once (the line logged for 255.44, and for a hand-set 253.37); 14 of 14 trials (both in at once, host
first, the guest at (4.61, 5.87) as in soak 5) with both out, no lock. (Guest-first trials were driver-blocked: the
two students push each other off the door spot.) QUICK PASS (cafe3q). Binary copies deleted.

## Weekly mysteries swept in a pair over the internet (2026-11-01, fix 15a671c)

Breadth check of the weekly mysteries (docs/COOP_GAME.md item 1) beyond the eight played in a pair before (5, 17,
31, 32, 34, 39, 40, 48). Internet mode on one machine (`--coop-internet`, `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`),
host `day1done.sav` (AAAAAA), guest `second_day1.sav` (NORA), Connect now, own binary copy (deleted after); harness
`/home/claude/run/sweep/` (`pair.sh`, `sw.py`: `test()` and `run_scene()`, `targets.py`: every start record of events
071-122 that starts a scene, keyed by its mystery byte +4, with its time window +0x22/+0x24 and the record's other
conditions). Set-up by pokes on the host (g1000/g1001, g1244 = 1, the mystery's own fields where the record asks,
the day's flag in both its places: 0x020AD7FE and field+0x3EB0) and a test step or warp next to the trigger; the
trigger itself by pad: walking into the area, through a town exit, or Up + A at a building's door (doors need A).
The other student was left where it stood: joined at once when near, else asked "Go and help?" and answered Yes.
Checked by the logs and RAM: the scene on the walker, the other told / asked / joined, both scenes ended, g1000/g1001
equal on both afterwards, both students free and moving. Clocks: Nov 2 10:00 and 20:00, Dec 3 19:30 (night windows, mystery 26), Feb 23 19:30, Apr 25 and May 25 10:00 (the
dated weeks 21, 50, 22: started by the calendar on the first town load, then the day's flag poked to 0 for the next day).

Rotation order of a fresh game (`obj.dat` byte +10): 14, 2, 29, 9, 1, 19, 43, 12, 7, 25, 6, 52, 42, 13, 32, 44, 27, 28,
41, 4, 17, 3, 11, 24, 35, 5, 36, 30, 33, 47, 16, 49, 8, 45, 10, 46, 18, 34. Dated: 15 (Jan 24-31), 21 (Feb 22-28),
23 (Mar 24-31), 50 (Apr 24-30), 22 (May 24-31), 51 (Jun 24-30), 20 (Jul 24-31), 37 (Aug 2-8), 38 (Sep 24-30), 31
(Oct 29-31), 39 (Nov 24-30), 26 (Dec 2-8), 48 (Dec 11-17), 40 (Dec 22-25). (Event file 097 is mystery 36 and 106 is
mystery 27; every other file is mystery = file - 70.)

| Mystery | Trigger tried (scene) | Walker | Result |
|---|---|---|---|
| 14 (rotation 1) | s1 area 15 (84.1); s2 town exit to map 63 (84.2) | host; guest | **pass**: s1 the guest asked, the scene (no text) over first: "already worked it out"; s2 the host asked, Yes, taken into map 63. 14/2 on both |
| 2 (rotation 2) | s2 area 15 (72.4), then the finale on the town's load (72.5) | guest | **pass**: host asked, Yes, joined; both played 72.5 ("I'm finally back to my original..."); equal stages |
| 29 (rotation 3) | s4 area 31 (99.5, g1039 = 209); s2 town exit to map 66 (99.3) | guest; host | **pass** both: the other joined; 29/6, 29/3 on both |
| 9 (rotation 4) | s2 area 16 in map 48 (79.2; warped into map 48) | guest | **pass**: host joined through the warp into map 48 |
| 1 (rotation 5) | s1-s4 areas 1-4 in map 63 (71.2-71.5; walked through the town exit) | host | **pass**: four short scenes without text, each over before the guest's Yes: offer withdrawn; stage 1 -> 5 on both |
| 19 (rotation 6) | no start record (started by talk) | - | not tested |
| 43 (rotation 7) | s1 area 56 (113.4) | host | **not reached**: wants 19:00-00:03 and the student's action 22 (record +0x36) |
| 12 (rotation 8) | s1 area 12, s2 area 17 (82.3, 82.4; 19:00-02:59) | host, guest, host | **pass**: s1 short, offer withdrawn; s2 the other joined both ways |
| 7 (rotation 9) | s1 area 11 (77.2), s2 area 7 (77.3), s8 town exit to map 64 (77.12) | host, guest, guest | **pass** all three: the other asked, Yes, joined |
| 25 (rotation 10) | area 32 in map 67 wants g1002 = 1 and action 46 | - | not tested |
| 6 (rotation 11) | s1 area 12 (76.5); s2 area 6 (76.3) | host; guest | s1 **pass** (guest near: joined at once); s2 **not reached**: action 22 (wizard form by the D-pad is 30) |
| 52, 35, 30, 45 | an incantation cast at the creature only | - | not tested |
| 42, 3, 10, 15 | no start record | - | not tested |
| 13 (rotation 14) | s0 the post office's door (83.1) | host | **not reached**: the door's own scene (255.50) ran, no entry (by day and at night) |
| 26 (Dec 2-8) | s1 area 6 (96.2, g1043 = 1, 19:00-04:00) | guest | **pass**: the guest (standing in the area when g1043 was set) started it, host joined; its g1043 = 0 taken by the host |
| 44 (rotation 16) | s1 area 68 (114.2) | host | **pass**: guest asked, Yes, joined |
| 27 (rotation 17) | s1 area 50 in map 41 (106.12, g1039 = 177; warped into map 41) | host | **not started**: no scene, a precondition not found (the record's listed ones were set) |
| 28 (rotation 18) | s1 area 60 (98.11, g1003 = 1) | guest | **pass**: short, offer withdrawn |
| 41, 18 | the student's own dorm room (not offered by design) | - | not tested |
| 4 (rotation 20) | s0 the school's door at night (74.1) | host | **not reached**: the door did not open at night |
| 11 (rotation 23) | s5 the books building's door, Up + A (81.23) | guest | **failed, fixed**: both consoles waited for ever at the principal's first line ("Well done, Master"; he had no model); also with the host declining; solo it plays through. After 15a671c: played to its end, host joined, 11/10 on both |
| 24 (rotation 24) | s1 area 26 in map 67 (94.2; warped into map 67) | guest | **pass**: host joined through the warp |
| 36 (rotation 27) | s1 area 46 (97.5); s8 the dorm's door, Up + A (97.7, g1039 = 216) | guest; host | s1 **pass** (host near: joined at once); s8 **failed, fixed**: both stuck at the first line (the warden 134 and the creature 216 had no model); after 15a671c played through, guest joined into the dorm, 36/9 on both |
| 33 (rotation 29) | s2 area 43 in map 37 (103.3, g1006 = 1; the warp's arrival is inside the area), then the finale on entering map 53 (103.2) | host | scene **pass** (guest joined through the warp into map 37, both taken to map 53, each ran 103.2); then the guest's student stood at (120, 0) in map 53 and could not move: **test set-up**, not co-op: 103.2 parks `actor[t171]` there and t171 = g1003 = 0 (the kid, set at stages 0-1 by 103.11, skipped by the poke), so actor 0, the student. Twice, the same |
| 16 (rotation 31) | s8 town exit to map 64 (86.28) | guest | **pass**: host joined |
| 8 (rotation 33) | s2 area 62 (78.2, by the dorm's door) | host | **pass**: guest asked, Yes, joined |
| 20 (Jul 24-31) | s1 area 17 (90.2) | guest | **pass**: host joined |
| 23 (Mar 24-31) | s1 area 27 (93.2, g1310 = 1); s2 town exit to map 66 | host | s1 **pass** (guest joined); s2 not valid on a Nov clock: the load dropped the out-of-date mystery (g1000 0 on both) |
| 51 (Jun 24-30) | s1 area 15 (121.2) | host | **pass**: short, offer withdrawn |
| 21 (Feb 22-28) | s0 leaving the dorm into town the day after it began (91.1; Feb 23 19:30) | guest | **pass**: host asked, Yes, joined; 21/1 on both |
| 50 (Apr 24-30) | s0 the books building's door, Up + A (120.5; Apr 25) | guest | **pass**: a step without text (variable writes only, not told by design), the host took stage 1; 50/1 on both |
| 22 (May 24-31) | s2 town exit to map 66 (92.31; May 25) | host | **pass**: guest asked, Yes, taken into map 66 and joined; 22/2 on both |
| 47 (rotation 30) | the school's door at night, Up + A (117.5 wants map 6 at 19:00-24:00) | host | **not reached**: the hall's own night scene (254.18) matched first and sent the student out |
| 49, 46, 18, 37, 38, 15 | Captain Dot's island by the air taxi / the haunted house / an area wanting a scene byte / no record | - | not tested |

**Totals**: 28 mysteries tried. A stage played in the pair by real input: 23 (1, 2, 6, 7, 8, 9, 11, 12, 14, 16, 20, 21,
22, 23, 24, 26, 28, 29, 33, 36, 44, 50, 51; with the eight before, 31 of the 52). Failed and fixed: 11 (81.23) and 36
(97.7), the same cause. Not reached by real input with this set-up: 4, 13, 27, 43, 47 (and 6's stage 2). Test set-up
artifact: 33's finale. No hang, crash or stage disagreement otherwise; in every pass g1000/g1001 matched on both
consoles and both students walked afterwards.

**The fix (15a671c, game/src/coop_class.cpp `scene_people_models`)**: in a session the building's placement gives a
scene's people no model and a talk waits for its speaker; the free-model hand-out that covered mystery 32's scenes now
covers every mystery's (events 071-122). QUICK PASS (sweep).

**For players** (what the sweep showed): a short step (a creature runs off, no text) is over before the other
player's "Go and help?" can be answered over the internet; the other then hears "Looks like <name> has already worked
it out." and the mystery has moved on for both. Longer steps are joined as before. A building's door is entered with A.

Harness notes: a test step across districts can leave the town without the target district's buildings (a void with
lamp posts); arrival points (`<entry> 0 0` in `MQ_MYST_GOTO`) load properly. Scenes need the day's flag 0 in
field+0x3EB0 as well as 0x020AD7FE (the host's own records read the field's copy until the next load).
`MQ_SCRIPT_TRACE` on one copy of an internet pair slows it so much that the link drops to its stand-in.

### Part 2 (2026-11-01, fix e610fc9)

Same method and harness, copied to `/home/claude/run/sweep2/` (`h33.py`: `hold_test()` for records that want an item
held, `setm2()`). Pairs at Nov 2 10:00 and 20:00, Aug 3 19:30. Two set-up facts found: **every mystery start
record is refused while s411 != 0** (ov001 0x020F42C8 reads s411 first: the principal has not yet told the student
about the Gateway; both test saves have it set; part 1 poked it only in later batches, which explains 27's miss), and
a record's **+6 = 1 wants the mystery's first day** (g1244 = 0; part 1 poked g1244 = 1). Action 22 (record +0x36) is
**holding an item up** (drag it out of the bag onto the open field), not wizard form.

| Mystery | Trigger tried (scene) | Walker | Result |
|---|---|---|---|
| 33 | s2 area 43 in map 37 (103.3), then the finale on entering map 53 (103.2); host t171 = 3 (as its own 103.97 sets), guest t171 = 0 (a guest that never ran 103.97 / 103.11 for the host's kid) | host | **failed, fixed**: the guest joined both scenes, then stood at (120, 0) in map 53 unable to move (103.2 parked `actor[t171]` = its own student). **Real**, not set-up. After e610fc9 both consoles' t171 = 3 at once, the same run: guest joined both, walks at (7, 10) in map 53, 33/3 on both. The stage-0/1 talk (103.1, actor 121) was not found: Sol's school talk is the lesson menu |
| 6 | s2 area 6 holding item 211 (76.3, action 22) | guest | **pass**: the scene started on the hold; the host was asked, Yes, but the guest's scene had moved the stage first ("the town has moved past the scene we were joining"); 6/3 on both |
| 13 | s0 the post office's door with g1244 = 0 (83.1, +6 = 1); s1 the door again (83.2) | host | **pass** both: s0 short (stage moved before the offer); s1 the guest joined. The first s1 entry ran the post office's own 216.90 (record 0, v20343 = 1) instead, once. s1's area 30 with action 17 (83.3): **not reached** (talk to the clerk, the stools: no action 17) |
| 27 | s1 area 50 in map 41 (106.12, g1039 = 177) with s411 = 0 | host | **pass**: guest asked, Yes, joined; 27/2 on both |
| 43 | s1 area 56 at 20:00 holding item 2104 (113.4, action 22) | host | **reached, pass**: guest asked, Yes, joined, both played it; 2104-2106 are the script's "wrong item" branch (item 29 moves the stage), 43/1 on both |
| 4 | s0 enter map 47 (74.4, +6 = 1): the door right of Sol upstairs leads to map 1, not 47; 74.3 (auditorium at night) warps into 47 | host | **not reached**: map 47 is reached only from 74.3 at night |
| 47 | s0 the school's door at 20:30 (117.5) | host | **not reached**: once the hall's 254.18 sent the student out (as part 1); once the student stayed in the hall with no scene: the door's own events (254.11, 255.96, by gdb on main 0x0201DAF0) and no map record. Alone (same night, poked the same) the student also stays in the hall with no scene: not a session difference |
| 8 | s2 area 62 (78.2: the letter, "The treasure slumbers on the new moon"), then s3: digging with the shovel at the letter's place | host | s2 **pass** (both ran it, standing in the area); s3 (78.10, action 49 after a dig): **not reached** by day (the dig at the letter's grid place found nothing; the clue names the new moon) |
| 37 (Aug 2-8) | started by the calendar on the first town load (g1244 0 on both); s56 = 1 poked (Mr. Vlad's festival talk sets it); into the haunted house (map 9) by warp; its rooms (13: 107.1, 254.36) | host | **not reached**: the inner doors did not open by Up + A in the time spent |
| 30, 35, 52 | an incantation cast at the creature (t163 = 36; t24 / t204; t160 = 3, t161 = 97) | - | not tested: words 36 and 97 are on the words screen's later rows (order.dat: 36 BASIC row 7, 97 category 3 row 7), past the four rows shown |

**Totals after part 2**: a stage played in the pair by real input for 34 of the 52 (part 2 adds 13, 27 and 43 to part
1's 31, and 6's stage 2; 33's finale now passes). Failed and fixed: 33's finale (e610fc9). Still not reached: 4,
47, 8 s3, 13 s1 (83.3), 37, and the cast-started 30, 35, 52. No hang or stage disagreement otherwise; in every pass
g1000/g1001 matched on both consoles and both students walked afterwards. QUICK PASS (e610fc9).

### Part 3: Captain Dot's mysteries 18, 46, 49 (2026-11-01, fix 6601c6c)

Internet mode (`NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`), host `day1done.sav` (AAAAAA), guest `second_day1.sav` (NORA),
clock 21:00, harness `/home/claude/run/sweep3/` (`pair.sh` link `sweep3`; `prep.py <mystery> <stage> [var=val]`:
s411 = 0, s914 = 0 on both, the stage, g1244 = 0 and the given fields on the host, 5000 Ritch, host into the café;
`talk.py`, `step.py` (one menu answer, then a shot of the bottom screen: mystery 18's menus are `$055'1`, shuffled on
each showing, so read from the shot; 46's and 49's are `$055`, fixed), `seq.sh` a whole answer chain, `fd.py` the
fight (X once each time Captain Dot starts a run at the student, state 2.0/2.1; in these runs X in his state 4 did not hit and his
dash then took a heart)). All by pad from the talk on; the guest answered "Go and help?" Yes, joined in the café and
answered its own copy of the questions. Answers (static, events 088/116/119, bank 32/33): 49 at stage 1 (g1002 = 1:
the kraken's talk) Yes, No, Yes, Yes; 46 at stage 1 (g1186 = g1188 = 2: the griffin's talk) the first option four
times; 18 at stage 2: Pay, 500 Ritch, Afternoon, Haunted house, Furniture, then **not** the wardrobe (t100 = 2 ends
it: "ya gots no proof"; a bookshelf or a vase makes him blurt out the wardrobe and fight).

| Mystery | Run | Result |
|---|---|---|
| 49 | 49a host lost (its harness late: no X), guest won | host 119.6 lost (room 22, g1001 = 2); guest 119.6 won, item 112 (the ring) in its bag, its g1001 = 5 not taken ("the town has moved on"): 2 on both. No hang |
| 49 | 49b host won, guest lost | host item 112, g1001 = 5; guest "Had enough" in room 22; 5 on both, both walk (`end49.png`) |
| 46 | 46a, 46b both lost | both 116.6 lost, rooms, 3 on both |
| 46 | 46c host won, guest lost | host item 122 and g1001 = 5; guest room 22; 5 on both |
| 18 | 18a both through the questions (both paid 500, 4500 left); the wardrobe answered | 869 "no proof" on both (a wrong answer, mine), stage 2 on both; **then the host stood in action 7 for good** (fixed, below). Battle not reached in the time left |

**Failed and fixed (6601c6c, `coop_talk.cpp`)**: the guest pressed A at the host's back just as the host talked to
Marcy across the counter (action 2: a text box, no event); the guest's console still saw the host free, so its
"came over to talk" reached the host inside Marcy's talk; event 90003 started into her open box, its line never
showed, the host stood in action 7 with an event running and A dead (10+ minutes, `stuck18_host_log.txt`).
Reproduced by both pressing A at once (`sim.py`); after the fix the same press: Marcy's lines first, then "NORA came
over...", the host walks. Our pending events now wait while the local student is in action 2 or 7 or an event
runs. QUICK PASS (6601c6c).

Not proven: both winning in one run (each console won once: the guest in 49a, the host in 49b and 46c; D4 above
did it for mystery 5), mystery 18's battle, the island rematches (088.12, 116.10, 119.3 at stage 2). Seen, not
followed: after its battle a guest sent to the host's room 22 shows the DS pattern on the top screen, not black as
the place list above says. Mysteries 4 and 47 not retried (time).

### Both students win Captain Dot's battle (2026-11-01, mysteries 49 and 46 at 200 ms)

Folder `/home/claude/run/battle2/c/` (sweep3's harness, link `battle2c`, `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`, host
AAAAAA, guest NORA, s411 = s914 = 0, g1244 = 0; `run46.sh <tag> <mystery> <stage> <pokes> <answers>`; the fights by
the battle log, `s3lib.py` `fight6`-`fight8`; `an/charges.py` tabulates every run of his against the casts).
How his battle works (MQ_BATTLE_LOG, alone and in pairs): he hops (state 1), then attacks: **2** a straight run at the
student's place (34 frames, stopping wherever it ends, 2.2/2.3 standing with flag 245 = 2), **4** a long homing run,
**5** a jump; ending within about 2.2 of the student takes a heart (state 9), otherwise he missed and stands dazed
(8.1, flag 245 = 2, ~60 frames). The magic flies ~2 units the way the student faces, so a hit needs him standing,
close and in front: (a) X as his straight run starts: he stops beside the casting student and waits (2.1, ~1.2 away),
X again then hits; (b) step aside from his run and, while he stands in 8.1, walk up facing him and X. **Alone the
student walks freely in this battle** (D-pad probe: (5.01, 14.22) -> (3.59, 15.64)); the solo win (`seq_h_s49c.out`,
item 112) came by (b).
- **Cause (co-op bug, fixed `coop_myst.cpp` battle_spot_frame)**: in a session the student was put back on the battle's
  spot (7, 14) every frame (built for the Gremlin/Nightmare, where the student stands): it could neither dodge his
  runs nor walk up to him in 8.1, so his long runs (4) always took a heart. Now in Captain Dot's battle the student
  walks as alone ("in Captain Dot's battle our student walks as alone"). Control with the old binary, same inputs
  (49j): guest won 3/0, host lost 0/3 (two state-4 runs, its 8.1 walk-ups went nowhere).
- **Also the test inputs**: Part 3's harness pressed X as each run started and seldom again while he waited
  beside the student, so many runs gave a heart away; hearts, positions and results never diverged between
  the consoles in a way that mattered (each console's own Captain Dot, own random hops; shots reach him on both).

| Run (fixed binary) | Host | Guest |
|---|---|---|
| 49f, 49g, 49h (dodge/walk-up harness, still crude) | lost (2 hits at best) | 49g, 49h won (item 112) |
| **49i** (fight8: (a) for runs, X at 4.2/5.2, (b) after a miss) | **won** 3 hits, item 112, g1001 = 5 | **won** 3 hits, item 112 |
| **46d** (same, answers 1st option x4) | **won**, item 122, g1001 = 5 | **won**, item 122 |
| **49k**, **49m** (same) | **won**, item 112, g1001 = 5 | **won**, item 112 |

- **Found on the way (fixed)**: at mystery 18 stage 2 the host's talk went into Captain Dot's **wager** in the café
  (event 057, outside the mysteries: "Let's do it", the battle 150.1, a bet item lost or item 433 won); it was taken for
  a mystery's battle and held the stage for 10 minutes ("the battle's result scene not seen" in the middle of 49k).
  A 150.1 that follows 057 no longer holds ("Captain Dot's wager (057): not a mystery's battle"); wager won (w1), 49m
  both won right after it.

QUICK PASS (both). Shots `/home/claude/run/battle2/c/win49m_host.png`, `win49m_guest.png`. Not done: mystery 18's
battle itself (its question talk was not reached: the talk at 18/2 offered the wager), 5's rematch. The host's result still decides the
stage (07e0daf): a guest that wins while the host loses keeps its item, the town stays at the host's stage.

### Part 4: the cast-started, haunted house, night school, battle and unplayed mysteries (2026-11-01, fix a8732a7)

Internet mode on one machine (`NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`), host `day1done.sav` (AAAAAA), guest
`second_day1.sav` (NORA) unless said, Connect now, own binary copy (deleted after); harness `/home/claude/run/sweep4/`
(part 2's `sw.py`/`h33.py`; `pair.sh` with `HSAV`/`GSAV`, `t4.py <h|g> <m> <s> <place>` (set-up, test warp, the
scene), `door.py` (Up + A into a door), `dropnow.py` (drag a bag item onto the ground), `dig.py`, `taxi.py`;
`c18/` part 3's battle harness for 18). Clocks: Nov 2 10:00 and 21:00, Aug 3 21:00, Aug 29 21:00, Jun 6 21:00.
Set-up facts found (static, then live):
- **`+74 = 0` on a walk-in record is the A button pressed inside the area** (matcher ov001 0x020F47C0: case 0 tests
  pad bit 0). Mystery 37's 107.2 starts only with A in map 13's area 63 (part 2 walked in without it).
- **s263 = 0 makes 010.7 (mc_ss block 6 record 4) match first on entering the school** (the test saves have 0), so
  the mystery records after it (4's 74.1, 47's 117.5) never matched: the night misses of parts 1-2. With s263 = 1 both
  start through the school's door by pad at night.
- **Store variables 256 and up are bit flags** (s411, g1280, g1283): a byte read of the store does not show them.
- **Actor records (act_u_ss)**: block = (mystery - 1) * 10 + stage - 1; +47 = the map (100, the town, when the
  decoder prints none); they match on taking a ground item (action 8/16 step 7, main 0x0202D4F0: t160/t161 = item
  / 100, item % 100, t163 = the item's kind, item record +6) or on putting one down (action 26, main 0x0202D550:
  t164/t165). Mystery 30's pages are items 323-328 (kind 36, looking like a raspberry, bone or ore), put in the town
  grid by 100.30 (323 at (7, 14)); 52's chest is 397 at (4, 14) on the island (122.11); 45's is the Special Rosso
  Coral (391) put down in Fungi Forest. Mystery 39's lily (207) put down in Fungi Forest started 109.3 in this pair
  (control).
- **Mystery 35's t24 = 3** is a result id written with t200 by main 0x02045500 (t204 = the person's index 0-11 in the
  field's people list): result 3 is the wand table's key 305 (`/magic/magic_wand_comp.bin`), not an item; X in
  wizard form at a person gave result 1 (t200 = 1, event 303.1), t24 not written. What gives result 3 was not found.

| Mystery | Trigger tried (scene) | Walker | Result |
|---|---|---|---|
| 30 | s2: the page 323 at grid (7, 14) in town (100.30 on the load, g1244 = 1): A facing it from (7.5, 15.5) | host; guest | **failed, fixed (c3dab24)**: A took nothing in a session (the story-item rule, below; the page is not drawn, alone too). After: the host takes it, 100.10 "Why, you have found one of my pages!", guest asked, Yes, joined, "A mere two pages to go.", 30/3 on both. The guest's own take: **open** (below) |
| 35 | s1: X in wizard form at a townsperson | host | **not reached** (above) |
| 52 | s2: the chest 397 on the island (reached by test warp and by the air taxi), A at (4.5, 14.89) facing it | host (NORA hosting, and AAAAAA); guest | **failed, fixed (c3dab24)**: in a session no name label and A took nothing; alone "Treasure Chest" shows and A takes it ("Yer a sharp one!", t160/t161 = 3/97, 122.12). After: the host takes it, 122.12, guest asked, Yes, warped onto the island, joined, 52/4 on both. The guest's own take: **open** (below) |
| 37 (Aug 3 21:00) | haunted house map 13 (test warp), A in area 63 (107.2; s54 = s56 = 1, g1244 = 1, day flag 0) | host | **pass**: guest joined at once, both played it, 37/0 on both |
| 4 (night) | s0: the school's door by pad (74.1, s210 = 0, s263 = 1); 74.3 entering the auditorium (test warp) | host | **pass** both: guest asked, Yes, warped in, joined; 4/0 on both (74.3's warp into map 47 wants query 0x66(46); here its other branch, g1070 = 1) |
| 47 (night) | s0: the school's door by pad (117.5); the auditorium (117.1) | host; host, guest | 117.5 **pass** (short, offer gone). 117.1 **failed, fixed (a8732a7)**: the walker's scene waited at the singer's first line for ever (no model, below); after: through on both, both ways, "You learned a new piece of music", 47/1 on both (`m47_pair.png`) |
| 18 | s2 at 21:00: host talks to Captain Dot (Pay, 500 Ritch, Afternoon, Haunted house, Furniture, a vase; g1283 = 1), guest Yes, joined, own answers (a bookshelf) | host | **pass**: both fought (map 70) and won, item 392 in each bag, both on the island, 18/6 on both, both walk (`win18_pair.png`) |
| 8 | s2 area 62 (78.2) at night; s3 the shovel at (12, 83), where 78.2 buries 30118 (grid 0x2076), Nov 2 and Aug 29 (new moon) 21:00 | host | s2 **pass** (guest joined at once, 8/3 on both); s3 **not reached**: before c3dab24 the dig there gave action 4 step 11 (the story-item rule: treasure 118); after it "You have dug up" (a furniture item), no 78.10; alone (AAAAAA, by day) the same: the moon's test not found |
| 13 | s1 area 30 (post office) wants action 17 (83.3: "What's this music?") | - | not tried: action 17 is playing a piece of music; how a student plays one not found |
| 41 | s2 map 7 area 23 with g1280 = 1 (111.5); s8 area 53 at 21:00 (111.6, wants 19:00-04:59) | host; guest | **pass** both: s2 short (offer gone, 41/8); s8 host asked, Yes, joined, 41/9 on both |
| 45 | s1: the coral (391) put down in Fungi Forest (115.3) | host | **pass**: guest asked, Yes, warped into map 66, joined; "This spot will not do." 45/1 on both |
| 25, 38; 3, 10, 15, 19, 42 | 25: island area 32 wants action 46 (s1) or 17 (s2-3); 38: the ruins' area 69 wants t190 1-4 and t191 = 1 (code-set); the rest talk-started | - | not tried |

**Totals after part 4**: a stage played in the pair by real input for 39 of the 52 (part 4 adds 4, 37, 41, 45, 47;
18's battle won by both; then 30 and 52 by the host after c3dab24: 41 of the 52). Failed and fixed: 47's 117.1
(a8732a7); 30's page, 52's chest (c3dab24). Open: a guest's own take of a story item. Not reached: 35, 8 s3, 13 s1. No crash; in every pass g1000/g1001 matched on both consoles and both
students walked afterwards. QUICK PASS (a8732a7).

**The fix (a8732a7, game/src/coop_class.cpp `scene_people_models`)**: the free-model hand-out for a mystery scene's
people left the classroom and the auditorium out (their classmates are the lesson code's); a mystery scene's own
person there (47's singer, 227) now gets one too while the scene runs. Seen: she looks different on the two
consoles (each its own free model).

**The story-item rule (c3dab24, `mq_myst_story_item`, a hook, so a regeneration)**: main 0x0203A2AC (item, area)
answers whether an item may be taken, dug up or named; it refuses only story items (30's pages 323-328 and 332, 8's
treasure 118, 52's chest 397 on the island at stages 2-3) while s411 is set **or in a session** (the DS's visitor
plays no mystery). Asked by the ground-object look-ups (ov003 0x02102C58 in the maps, ov001 0x020CB306 in the town),
the shovel (main 0x0202A8BC) and a script query (ov028 0x02114C32). Found by gdb against a solo copy (the look-up's
calls compared, a gdb FinishBreakpoint on each callee). In a session it is now asked as alone (the session pointer cleared for the
call); s411 still counts. QUICK PASS.

**Open: a guest's own take of a story item.** The guest's student takes (action 8), asks the host (request 0x34,
answered by ov001 0x020F07B0 from the host's own grid or map store) and at step 6 reads the answer: the chest was
answered no (50; the host had not loaded the island at that stage, so its store held no chest), and the step went
6 -> 11, no t160, no scene; the page was answered yes (0x1D) and taken off both grids, yet the guest got no item
and no scene (not followed further: a second try at placing the page in the same pair failed, 100.30 not run).
The host's take works and the guest joins.

Seen, not followed: after 18's battle the guest's top screen on the island at night shows a striped picture
(`win18_pair.png`, right); the host's shows the sky.

### Part 5: a guest's own take, the guest's top screen, 47's singer, talk-started mysteries (2026-11-01, fixes 7ab88e8, 684caed, f19aad5, 7c41bf5)

Same set-up (internet mode on one machine, `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`, host `day1done.sav` AAAAAA, guest
`second_day1.sav` NORA, Connect now, own binary copy, deleted after); harness `/home/claude/run/sweep5/` (part 4's,
plus `g30.py` both students in town then the guest's take, `p47.py`, `isl.py` / `b70.py` / `cafe.py` / `rt.py`
(screens across field changes), `tk.py` / `tkq.py` (a mystery's person: set-up by the town's load, the walker set
beside it with the warp file's `a <person> <dz>`, then A), `t25.py`). Research switches added: `MQ_SCREENS_LOG=1`
(the display swap, DISPCNT A, 3D on both screens, the sub engine's layers, the area, on each change) and
`MQ_DUAL_FIELD_KEEP=1` (the screens' set-up kept across field changes, as before 684caed).

**A guest's own take of a mystery's item (fixed 7ab88e8).** Reproduced with both students in town at 30/2 (each
console's 100.30 had put page 323 at (7, 14)): the guest's A went action 8 step 0 -> 11 -> 12 -> 13 -> 0, item -1
at the student's +0x55E, no t160, no scene, and the page was gone from both grids. Cause: a guest's take (field
object +0xAA = 1) picks the item through main 0x0202DCA8, which keeps it only if the item's record (main 0x0203975C)
has bit 0 of +8 (the items a visitor may take, 107 of 1634); the story items lack it. The host's answer to the
take's request 0x34 (ov001 0x020F07B0) uses the same bit and its own grid or map store, which holds 52's chest only
if the host's own 122.11 put it there. Now in a session the story items' records get the bit (put back after the
session) and the host answers a story item's take yes (`mq_myst_arb` / `mq_myst_arb_sent`). After: the guest takes
the page (100.10 "Why, you have found one of my pages!", host asked, Yes, joined, "A mere two pages to go.", 30/3 on
both) and, on the island with the host in town, the chest (122.12 "Yer a sharp one!" to "Take care of yerself!",
"slot 1's take of story item 397: answered yes", host warped onto the island and joined, 52/4 on both). 8's
treasure (dug, 118) is covered by the same bit; its stage 3 is still not reached (the moon's test).

**The guest's top screen (fixed 684caed): one cause for two of the three reports.** Two faults of ours on a field
change: (1) the next field is often made at the last one's address and its screens are set up well after it is up
(entering the auditorium: area 8 from frame 9630, its set-up at 9789), so in between the per-frame code for 3D on
both screens treated it as the last field (a room entered from the town got the town's label move: the talk menu's
labels to the sub engine, the top screen, seen in the auditorium with `MQ_DUAL_FIELD_KEEP`); (2) leaving a room whose
top screen showed the session pattern (BG1 of the sub engine, loaded by us or the game) for a field with 3D on both
screens by a scene's warp, BG1 stayed shown with the pattern's map and tiles: the top screen showed the capture read
as BG1's tiles, **the striped picture**, and the bottom screen a band of the pattern over the view. Reproduced by
warps on the guest: auditorium -> island (`isl_fix.png`: stripes and band), auditorium -> town and Lovers Point (the
band, `rt_both.png`); mystery 18's path is the café (pattern) -> Captain Dot's battle map 70 (3D on both screens)
-> the island. Fixed: the set-up is forgotten when the field goes or the area changes (`mq_field_screens_set` for
code that must wait for it) and BG1 is hidden once a field with 3D on both screens is set up after a room. After:
auditorium -> island (`isl_fix2.png`), café -> map 70 -> island (`b70_fix.png`: the hiding logged in area 70), the
café by its door in and out and auditorium -> town by warp (`cafe3.png`) all right; labels moved only in fields with
3D on both screens. Not reproduced: the Captain Dot talk "on the top screen with no 3D" (D3/D4 of 2026-10-31); the
label move in a room is the likely way it happened (fixed above). Once, the host after 117.1 showed its screens
swapped (the main engine on top, the pattern below) on the binary before these fixes; not seen again in four runs.
The guest's room 22 after a battle showing the DS pattern is right: rooms show it since 2026-10-28 (docs/COOP.md,
"The DS's session pattern in rooms"; the place list's "black" is single player's).

**Mystery 47's singer (fixed f19aad5).** Alone she is blonde with a pink headband (`solo47b.png`). In the pair the
console whose student walked in made her model as 117.1 began, before the auditorium's set-up ended 3D on both
screens, and she kept the last person's colours in that model (the mail cat's purple, her thought bubble too); the
other console made hers after the set-up and had them right. A model made before the room's set-up is now made
again once the screens are set up ("model 2 made again after the room's set-up"): blonde on the host walking in
(`h47fix.png`) and on the guest walking in (`w47_g.png`). (The same remake ran harmlessly for the air taxi's driver
on the island.)

**Untried mysteries, one pass each.** The talk-started ones are set up by the town's load at their stage (mc_ss
"enter map" records: a person placed), then started by talking to that person.

| Mystery | Trigger tried (scene) | Walker | Result |
|---|---|---|---|
| 10 | the person 190 (080.1 places, walks about); A beside him (080.2) | guest | **failed, fixed (7c41bf5)**: the guest's 080.1 ran unseen within a frame and was vetoed like a scene for 20 s, so the next town load placed no one; set-ups are no longer vetoed. After: 080.2 (he speaks backwards), host asked, Yes, joined, 10/1 on both |
| 19 | the person 199 (089.1); A beside him (089.2) | guest | **pass** (start): the tag game ("Catch me if ya can!"), host joined, g1010/g1012/g1054/g1070 to the host; the chase itself not played |
| 42 | the person 222 (112.1, sets 42/1); A (112.2) | host | **pass**: stage 1 short (the guest's offer gone), the next talk at stage 2 joined by the guest, 42/2 on both, both walk |
| 25 | entering the island 16:00-24:00 (095.1, test warp at 21:00) | guest | **pass**: 095.1 moves 25/0 -> 1 on both (host's offer gone: short). s1's area 32 (action 46) not tried |
| 3 | Principal Sol (121) atop the books building's book tower (073.4 places him, y 5) | guest | **not reached**: the climb by pad not found |
| 15 (Jan 25) | the person 195 (085.1) | guest, host | **not reached**: she is placed and walks, A beside her started nothing on either console (no text); alone not checked |
| 38 | area 69 wants t190 1-4 and t191 = 1 (set by code only; 108.3 reads them), Sep 24-30 | - | not tried |

**Totals after part 5**: a stage played in the pair by real input for 44 of the 52 (part 5 adds 10, 19, 42, 25; 30's
and 52's items now by the guest too). Fixed: 7ab88e8, 684caed, f19aad5, 7c41bf5, each QUICK PASS. Open: 3, 15, 35, 38,
8 s3, 13 s1, 19's chase, 25 s1; mystery 18's battle path itself not replayed (its field sequence was, by warps).

### Part 6: mystery 19's runner, Daisy's display, 3, 15, 38, 35, 13 (2026-11-02, fixes 27f8909, 316f036)

Same set-up (internet mode on one machine, `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`, host `day1done.sav` AAAAAA, guest
`second_day1.sav` NORA, Connect now, s411 = s914 = 0, g1244 = 0); own copy of the tree and binary (deleted after);
harness `/home/claude/run/sweep6/h/` (soak 6's s19 harness renamed: link `sweep6p`, port 49961, names w6h/w6g;
`setup.py` the mystery and both through the town's load, `chase.py` / `tag.py` by pad to a moving person with a test
step beside him when far, `display.py` Daisy's counter, `tk.py` / `tk3.py` a talk-started mystery, `t13.py`).
Research switch added: `MQ_TALK_LOG=1` (each talk try: every NPC within 4 units with the talk finder's tests).

**Mystery 19's runner (the report "nobody can talk to him in a pair"): the test tool, plus one co-op bug, fixed.**
- The talk finder (ov001 0x020C85E4) wants the person within 1.4 units of the student's height (0x1667; 0 for
  most people). The warp file's `a <person> <dz>` set x and z only: the student kept the height of the place it came
  from (3.0, the café's front) and hovered there over the lower ground (0.0) where the runner ran: every probe said
  `dy -3.00`. Alone the same loop works only where both grounds are 3 (soak 6's third try). The warp now sets the
  person's height too (docs/TESTING.md). By pad (a test step 2.5 units off, then walking) both consoles talk to him:
  the host's and the guest's 089.2 ("Huh? Never seen you 'round" ... "Catch me if ya can!"), the other asked, Yes,
  joined, g1010/g1012/g1054/g1070 the same on both; a catch (089.10, "Yikes!!! Ya got me!") on the host.
- **Fixed (27f8909, `coop_npc.cpp` note_own_moves)**: 089.2 ends by sending him to his farthest spot. On a guest the
  host's copy decided him, so as soon as the guest's scene ended its copy was put back at the host's copy's place,
  beside the guest's student ("NPC 199 put at slot 0's place (67.35, 27.63), 7.94 units away"). A person a guest's own
  scene moves in one go is now that guest's for 60 s (renewed by each move): it tells the host (entry busy byte 2) and
  the host's copy follows ("NPC 199 moved by this console's scene to (10.48, 12.88): its own for now" on the guest,
  "NPC 199 claimed by slot 1's scene" on the host). The host's own events still come first.
- **Open: the tag game ends at once in a pair.** Right after "Catch me if ya can!" the runner's trigger (command 108,
  /npc/NpcCmd.bin: a motion 117 for 90 frames) fires 3-4 times and 089.30 starts ("Whoa! That was close!" ... "Ha ha
  ha! In your face!"), g1010 = g1011 = 0: lost, on the host walking too. Seen with it: a mystery person's job start sets
  g1042 = 1 (ov001 0x020E1D94, any NPC 181-244) and the person's update 2 (main 0x0202FFEE), a host variable here (a
  guest's write is put back); keeping it the guest's (tried) did not stop 089.30: the game itself sets it 0 at once.
  Alone it was not reached (the pad driver never caught up with him), so whether 089.30 at the start is the game's
  own with these inputs is not known. 089.30 also comes by an A pressed at him while he is not catchable.

**Daisy's display (soak 6's +0x4E38): fixed (316f036, `coop_sync.cpp` emptied_places).** Her counter's display is the
room list at +0x4E24 (u16 item | 0x8000, u16 place: 0x0286-0x0288 the three on the counter). The buyer's console
empties the item (0xFFFF) and keeps the place; a console that hears of another's purchase empties both (place 0).
Reproduced at Jul 18 14:00 with the guest in the shop: host 80c7 -> ffff at +0x4E36, the guest the same plus +0x4E38
0x0287 -> 0, "4 bytes this console changed by itself, kept here", different for good. Now a place a guest's game zeroed
behind an item it emptied is put back to the host's (rooms' lists +0x4800..+0x6000): "1 emptied place(s) of the
rooms' lists as the host has them: +4e3c=0288", both lists equal 8 s later. The guest's own purchase (before the fix
too): its item ffff, the host's game writes place 0, sent: equal.

| Mystery | Trigger tried (scene) | Walker | Result |
|---|---|---|---|
| 38 (Sep 25) | 108.1 places actor 218 at (12, 80); A beside him (108.2) at stage 0, then stage 1 | guest; host | **pass** both: the other asked, Yes, joined; "You memorized" a piece of music; 38/1 on both, both walk. Stage 2 (the ruins' area 69, t190 1-4 and t191 = 1: playing that music there) not tried |
| 15 (Jan 25) | 085.1 places 195; A beside her (085.2) | host | **pass**: the guest asked, Yes, joined ("So tell me, are you a good kid"...), 15/1 on both, both walk. Part 5's "A started nothing" was the same height artifact as 19 |
| 3 | 073.4 places Principal Sol on the book tower in the bookshop (map 53, y 5, 4:00-18:59); the guest in by the door, beside him, A | guest | **talk reached** ("Ah! If it isn't NORA!", his menu, the hint "One of the books is bit wild"): his own talk, not the mystery's scene. The scenes are 073.2/073.3 with the wild book (actor 183; 073.3 begins with query 0x28(2152)); what starts them was not found. No hang, 3/0 on both (`sweep6/h/sol3_g.png`) |
| 35 | 105.2/105.3 want t24 = 3 (a cast's result) at a person (t204): the tapir, a dream eater ("*gobble gobble gulp*") | - | not tried: the incantation that gives result 3 is still not known |
| 13 s1 | the post office's door by pad (83.2) | guest | **pass** on the second entry (the first ran the office's own 216.90, as part 2 saw): host asked, Yes, joined in map 61, 13/1 on both, both walk. 83.3 (area 30, action 17: playing music) not tried |
| 8 s3 | - | - | skipped: fails alone too (part 4, the moon's test) |

No crash, no hang; g1000/g1001 equal on both after every pass. Not done: 19's tag game to its end (open above), 3's
and 35's scenes, 38 s2 and 13's 83.3 (both want a piece of music played).

## Soak 6: internet mode at 200 ms, shops bought and sold, the café's payments, mystery 18 replayed (2026-11-01, fixes 803b4c6, 0f87040)

Binary of 7e2b898 plus the fixes below (own copies, deleted after), `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`, host
`day1done.sav` (AAAAAA), guest `second_day1.sav` (NORA); driver `/home/claude/run/soak6/soak6.py` (soak 5's plus
`shops`: Daisy's by her menu's Buy berries or a display, the Emporium's Sell by dragging the newest bag item to the list;
`cafe2`: a drink each with money checked; `harrows`), logs `soak6/logs/`, shots `live/shots/soak6/`. Two segments, about
73 minutes: Jul 18 14:00-16:40 (town together, a chat each way, shops, café, Harrows, a walk, the seven buildings, the
host's Disconnect, both "Save complete."), then from those saves Jul 19 02:55 (the day turned at 3:00 in the session,
both connected again ~20 s later, a walk, a chat each way, the dorm, Disconnect and save). 152 state comparisons and 755
copy samples: **0 fatal lines, 0 stalls, 0 frame-rate drops, 0 copy mismatches**; town block, date, weather and `.coop`
records equal throughout except the two items below. Resident memory flat: main 341.3 -> 343.3 MB over 48 min, the
stand-ins 349.9 / 350.2 MB unchanged (segment 2: 342.0-342.3). Guest top screens read in the shots: right (rooms the
pattern, town the sky, the island's clouds).

**Purchases took no money (fixed 803b4c6, `cfie_hooks.cpp` mq_text_box_draw).** Daisy's berries: host 3000 -> 3000
with item 384 in the bag; the guest the same. The price is taken by a code at the end of the line ("All right, then.
That'll be 100 Ritch.$058'-100;"; the café's "Today's special ...$058'-200;", Captain Dot's 500): it runs when the
window's printer (window +0x17C4) reaches it. Fast text (the default) only drew the box ahead of the printer, so the
line looked finished and A came early: the box closed and the code never ran. A 20 frames after the box opened:
normal text 2950 -> 2850, fast text 2850 -> 2850. This is soak 4's "the host's order, money stayed 3000" (which one paid
depended on the driver's timing). Now fast and instant text move the printer itself, one character a step, stopping
where the game's step stops. After, in the soak: Daisy's berries host 3000 -> 2900, guest 3100 -> 3000 and 3000 -> 2900;
displays (50) 2900 -> 2850, 2880 -> 2830; café host 2830 -> 2630 and 2630 -> 2430, guest 2900 -> 2700; Captain Dot's
500 on both. Emporium sell: a White Tulip 2850 -> 2880 (and 2730 -> 2760). **The regression's base must be remade with
this before main**: QUICK s10000 same, s20000 differs (the welcome's lines finish sooner, so the input script's A
presses meet finished lines); a run with text speed normal is unchanged.

**Mystery 18 (task: confirm 684caed).** Both through Captain Dot's questions in the café (host Pay 500, Afternoon,
Haunted house, Furniture, a vase; the guest joined and answered its own, a bookshelf), both won the battle on map 70
(item 392, 18/6 on both), both on the island at night: the top screens show the clouds and rain, no stripes, on both
(`soak6/c18/isl18_pair2.png`). **Found on the way (fixed 0f87040, `mq_dual_session_top`)**: the guest's questions came
on the top screen over black, the pattern below (`c18/swap_g_18.png`); then the host too after its map warp in. A room
entered by a warp/join while 3D on both screens was on: the end of 3D on both screens left the last pass's display
swap (1) and the main engine's layers without BG0. Put back once the room is up; after, both consoles the pattern on top
and the café below through the whole talk (`c18/m3.png`-`m6.png`).

**Seen, not fixed**
- *Daisy's display purchase, town bytes +0x4E38 / +0x4E3C*: each host purchase from a display adds a u16 pair
  (0x0287, 0x0288) there; the guest's game clears them by itself and keeps its own value ("4 bytes this console changed by
  itself, kept here"), and since the host's value does not change again it is never sent: different until the session
  ends (COOP_GAME.md's "stock written outside the store"). Effect on the guest's shop not checked. (Fixed: part 6, 316f036.)
- *Harrows, the host's copy on the guest 1.1 off for 20 s* while the guest stood at the entry in action 7 (the
  greeting): the copy drifted from 0.98 to 1.08, the guest's student pushing it. Gone when they left.
- *Clock 3 minutes apart* from 21:18 on: the harness's unpaced guest ran ~2% fewer frames (190452 vs 199122), and the
  game's clock follows frames. Paced at 60 fps both run alike; not a fault seen in play.
- Driver: the guest's Emporium door was blocked by the host standing on its spot (leaving now steps aside); a guest drink
  became the players' talk (the host by the stool); the dorm on day 2 and Harrows once missed by the guest (left in a
  talk); 237/238's exit from x 6 (pushed aside).

**Mystery 19's chase: not reached, a lead** (part 6: the lead was the warp tool's height; talks work in a pair). At stage 0 the town's load (089.1) puts 199 out and moves 19 to stage 1;
he runs about (several units a second). Alone (AAAAAA, Nov 2 10:00, the warp file's `a 199 0.9` then a facing and A at
once, repeated) the third try talked to him ("Huh? Never seen you 'round"). In a pair at 200 ms the same loop got no
talk in 40 tries on the guest and 20 on the host (the other student walked away first), also with
`MQ_COOP_NPC_GAME=1`; `MQ_MYST_ARB_LOG` showed no talk request at all, so the finder did not take him (part 5's start by
the guest was not repeated). Harness `/home/claude/run/soak6/s19/` (part 5's, `tk.py g 19 0 199 0`). Probably the same
as 15's "A beside her started nothing" (part 5). 3, 15 and 38 not tried further (time).

## A shared battle: one Captain Dot for both students (2026-11-02, opt-in `MQ_SHARED_BATTLE=1`, exploratory)

Folder `/home/claude/run/shared/` (`build.sh`: a private copy of the tree built in its own mount namespace; `h/`:
battle2/c's harness renamed, link `sharedb`, port 49911, names shh/shg, `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`, host
AAAAAA `day1done.sav`, guest NORA `second_day1.sav`; `pair.sh`, `prep.py 46 1 1186=2`, `seq.sh h <tag> talk 3.3 5.1 0 0 0
0` / `seq.sh g <tag> yes 0 0 0 0` (mystery 46, the talk by pad, the guest's "Go and help?", the fight by `fight8`),
`shots.py` the pair's screens in the battle). Design: docs/COOP.md "A shared battle".

| Run | Result |
|---|---|
| s1 | the guest never followed: the host's state message is 28 bytes, the guest wanted 30 (fixed) |
| s2 (position, hearts, flag 245 only) | the guest's copy at the host's place, the host's 2 hits taken down on the guest; the guest's shots reached it (check 1) but never took a heart: its copy's own state (hops) was not the host's |
| s4 (+ the host's state) | **the guest's hit counted on the host** ("the guest's hit 1 taken: 3 -> 2"), the host's 2 on the guest; at 0 the guest's copy stayed in state 7 with no result, stuck in the battle (fixed: the host's result 2 taken) |
| **s5** | **both won together**: guest 1 hit + host 2 hits on the one Captain Dot, results 1 s apart (host 3/0, guest 1/0), item 122 and stage 5 on both |
| s6, s9 | the host's student lost its hearts (result 1); the guest then fought its own copy (s9: won alone, item kept, the host's stage as before) |
| s7 | the host's messages came in bursts with gaps over 45 frames (a lost message's resend holds later ones back): each gap made the guest "rejoin" and count a host hit as its own (its copy at 1 heart, the host's at 2: `fight_s7_*`). Fixed: 180 frames, the guest's earlier hits counted only on the first hearing |

Seen: the guest's copy follows the host's ~5-8 frames late at 200 ms; a guest hit lands where the guest saw him and
counts (the shooter is favoured). The guest lost hearts to runs it saw end ~2.7 away (its copy's own run between the
frames it is put back). The host lost 5 of 7 runs (its own fight unchanged; the fight driver's timing around the
flinches the guest's hits cause is the likely cause, not followed). Not tried: the Gremlin, the Nightmare, real
players. Quick regression: the switch's binary and a control of 6030a8c give the same shots (both differ from base/
at s20000 since the text speed change 803b4c6).
