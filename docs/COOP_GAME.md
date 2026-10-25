# The whole game in co-op: catalog of systems

What each of the game's systems is, where it lives, what a co-op session does to it today, and what co-op should do
with it, so that "the entire game" can be played together. Static research only (ROM data, event scripts, the
disassembly); nothing here was run. [V] = read in the data, scripts or code; [I] = inferred, to check.
Background: docs/COOP.md (the session, side channel, town sync, classmates, refusals, the night) and
docs/COOP_RELATIONSHIPS.md (players' friendship and romance).

**The model the plans keep** (docs/COOP.md): not lockstep. Each console runs its own student and its own copy of the
host's town; scripts, menus and text run only on the console whose student is in them; the host's town block is
kept in step by the town sync (guest writes to the global store go to the host as requests; guest rolls are never
sent). "Host decides" below means: the host's console rolls or picks, the result travels in the town block or a side
channel message, the guest only reads it. "Both join" means: the scene runs on both consoles at once, each for its own
student, started together. "Each does own" means: the local student's own business, already right when nothing of
the host's town is written.

**Whose world.** In a session the host's town is the world. The guest's student record (money, bag, magic, lessons,
relationships with classmates, titles) is the guest's and is saved home at the end; anything the guest's actions write
into the host's town (the global store, variables 1000-9999 and >= 30000) stays in the host's town, and the guest's own
town does not see it. Several plans below depend on this: a town-level progress flag the guest earns in the host's
town (a mystery solved, a festival held) is not in the guest's own town when they go home.

## Priority table

Ordered by player impact (how much of a normal day or week of play it touches, and how broken it looks today). Size:
S (a hook or a list entry, under a day), M (a few hooks and a side channel message, days), L (new shared-scene
machinery, a week or more).

| # | System (player-facing) | Where it lives | Scope | In a session today | Co-op should | Size | Status |
|---|---|---|---|---|---|---|---|
| 1 | **Weekly mysteries** (a strange creature or happening in town each week; the main story) | events 070-122, 258; start records `date=N/s` against g1000/g1001 (ov001 0x020F42C8); new mystery ov001 0x020CE830, stage at day roll ov027 0x02118CD4 | town | Runs on the console whose student walks into the trigger; the other sees nothing of it; its stage writes (g1001, g10xx) from a guest go to the host's town as requests | Both join each stage scene; host decides the stage; each gets the rewards | L | **built** (COOP.md, "Mysteries together"); open: battles inside mysteries, a stage started by its own trigger not seen in a test |
| 2 | **Daily lessons** (pick a class from Principal Sol, the class, questions, house points) | menu ov038 (g1017 at 0x0211C2B2); classroom start records mc_ss block 0 (g1017 = 1..96); events 011, 021, 031, 041, 042; end writes g1015 = 1, g1017 = 0, g1520-1523 | per student, through two town variables | Built: the lessons start together (the first in waits for the other), the other student sits at the desk beside, each its own lesson and grades; two classmates in class (the free model slots) | Both join: same room, same time, each its own lesson text; partner seated as a classmate; g1017 local to each console | L (S for g1017) | **built** (COOP.md, "Lessons together") |
| 3 | **School festivals and holidays** (spring tea party, test of courage, mushroom hotpot cook-off, rainbow snow festival; New Year, Christmas, Halloween) | events 500-503 (announced in class, held 2nd-8th of May, Aug, Oct, Mar), 504, 505 (Jan 1-8), 110 (Dec 18-25), 101 (mystery 31); calendar bytes +28..+32 of start records | per student, done flags in town (g1387-1390, g1242) | Per console; the guest's done flags land in the host's town; nothing shared | Both join the announcement and the judging; each scores itself | M-L | done flags the host's alone (built); joint scenes open |
| 4 | **Refused incantations** (summons and transforms; Connect doors, Sweet dreams, Make peace, Sit by me, Honor student, Mushroom) | flag 212 tests in events 332, 350-353, 355-357, 359 (999:3120); query 0xB5 refusals 3114-3119 | per student (some change the town) | Refused with "cannot be used during wireless communications" | Each does own, with the effect sent to the other console (a creature or a transformed student must show on both) | M | **built** (all but Phantasm; docs/COOP.md, "Incantations in a session") |
| 5 | **Evaluation and magic tournament** (yearly titles; the auditorium tournament) | 590 (s507, auditorium map 8, credits ov025 via query 0xA0); 520, 521, 530 (s225; query 0xB1 -> ov009 + ov034) | per student | Per console; each student fights or is judged alone | Both join; the tournament pairs the two players when both entered | M-L | open |
| 6 | **Gateway destinations** (Anywhere, Mystery time, The other world) | event 250 block 1 (5321 / 5331 / 5341; queries 0x96, 0x45, g1000 = 32) | per student | In a session the Gateway offers only the communications room and Exit (5316) | Host decides the destination; both travel | M | open |
| 7 | **Insects, fish, mushrooms** (catching, the books) | ov001: insects field+0xA74C (82 RNG calls), fish (16), mushrooms (6); tables map/insect, map/fish, map/mushroom | town (spawns), per student (catches) | Each console rolls its own (RNG not shared): the partner sees other bugs and fish | Host decides spawns; catches claimed through the host as ground items are | L | registration lifted |
| 8 | **Classmates' life events** (transfers in homeroom, the farewell, classmates visiting your room, after-class invitations, dates) | g1061 (rolled ov027 0x021161B6..; 550 farewell, 551 / 555 homeroom); 400-402 (rooms 22-36), 254 blocks 60-63; 010, 580-584; 700-705 | town (roster) and per student | Homeroom plays for whoever enters first and is marked done in the town; the farewell (551) runs on both consoles (it hung: its leaving classmate had no model, item 2; fixed); room visits off in a session (flag 212); the rest per console | Host decides the roster; both join homeroom; each keeps its own visits and dates | M | g1061 the host's alone (built); joint homeroom open |
| 9 | **Small refusals**: storage chest, school bell (chime), tatami room, bank's session menu | 225 block 1 (999:1021, ov017); 221 block 12 (8614, ov013); 254 block 45 (5306, area 15); bank menu 5808 (32:5860) | per student / town (bell) | Refused | Each does own (tatami room, bank); host decides the bell's melody; the "chest" is the trash can, a town list: host decides | S each | tatami room lifted (built); the "chest" is the trash can (town): stays refused |
| 10 | **Town-changing magic and weather** (Rainbow, Flower rain, Rainmaking; the day's weather) | 310, 313, 334 (host's slot writes g1100/g1101; g1400); weather store 540-559 (ov001 0x020CFBC0-0x020CFE30); query 0x16 | town | The game's own two-player path (packet 0x45, the other console ready); the guest's sky follows the host's bytes only when it reads them again | Host decides; push a sky refresh on the guest | S-M | **verified** (the game's own path works in a session; the host's weather bytes sent at once; the guest's sky redrawn at its next load) |
| 11 | Shops, bank, post office, salon, boutique; part-time work | 211-219; ov005, ov008, ov014, ov018 | per student (stock: town) | Purchases and work lifted (COOP.md); stock written outside the store is not sent | Each does own; host decides stock | M | mostly built |
| 12 | Telegrams, letters, the bulletin board | post office 216 (ov009 + ov035, query 0x76/0x8A), packet 0x42; board ov016 (bank 998 news) | per student / town (board) | Telegrams have a session packet; ov009 and ov016 read the session [V] (what for: [I] keep the link alive) | Each does own; a telegram to the other player | S-M | to verify |
| 13 | Extracurricular classes (creature hunt, incantation experiments) | 511-516 (s233/s234), 517-519 | per student | Per console | Both join | M | open |
| 14 | Players' rooms, houses, furniture | dorm rooms areas 22-25 (query 0x26: another player's room is "full"); apartment map 42 rooms 26-29; houses on the town grid | per student | A guest enters only its own slot's room in the host's dorm; the guest's furniture and house are not in the host's town | Each does own: show the guest's own room from its record | L | open |
| 15 | Outdoor areas (maps 63, 64, 66), noodle stand, haunted house | doors 255 blocks 57-78; noodle stand 227 (map 64 or 66 by g1516); haunted house map 9, rooms 10-13 | town | Open (no session test); mystery 31 closes them ("You are unable to enter"); rooms hold at most 3 students (query 0x26) | Nothing new beyond 1 and 7 | S | works [I] |
| 16 | Day roll, calendar, the night | ov027; town date +0x2C8; ov001 0x020C8CD4 | town | **Built**: the new day ends the visit, each town rolls at home, both connect again | Host decides | - | **built** |
| 17 | Punishment for turning off without saving (cleaning) | s500; 801-804, 257 (maps 68, 69) | per student | Checked: closing during a visit is not punished (the visit's start clears the save's flag); closing after the visit's end-save and before home (end or error messages, the new day's wait) is | Never punish a co-op error | S | **built** (`<save>.visit` marker, COOP.md) |
| 18 | Pupil council, clubs | none in the data (no text, no scripts) | - | - | - | - | n/a |

## 1. Weekly mysteries (the town's event schedule)

**What it is.** Each week a "mystery" happens in town: a creature appears (the UMA, Unidentified Mysterious Animals),
a ghost, a pirate, Santa, Halloween trick-or-treaters... and the student solves it with magic over several days and
stages. Up to 60 numbers; 52 have event files (071-122, one per mystery; e.g. 101 "Trick or treat?" is mystery 31, 110
"Ho ho ho!" mystery 40, Dec 18-25). Event 070 (60 blocks, map 71) and 258 (54 blocks, map 44, query 0x63 -> ov004
`/sys/umatbl.bin`) place each mystery's creature (actors 181-240) for the creature catalog [V].

**Where.** Start records carry `date=N/s` (record bytes +4/+5): ov001 0x020F42C8 compares them with global store fields
0 and 1 (g1000 = the mystery number, g1001 = its stage; fields 50/51 when N > 60) and first requires student field 411
to be 0 [V]. A new mystery is started by ov001 0x020CE830 (g1000 = n, field 0xF2, and the stage fields 1.. cleared,
0x020CE89C) [V]; at the day roll ov027 0x02118CD4 copies g1025 into g1001 (the next day's stage) [V]. The scripts move
the stage on with set_town (g1001, g1003, g1010, g1012, g1054, g1056...) and use query 0xAE at the start of scenes:
in a session it sends packet type 0x1A to the others and waits until sent (ov028 0x02119BA0) [V]; what the others do
with it is not traced [I: the student shows as busy].

**In a session today.** Records match on both consoles (the town block is the host's, so g1000/g1001 agree), so a
mystery's actors appear on both; but each record's script starts only where its trigger area (selector) is entered,
on that console, for its student. The partner sees the other student stand still. The stage writes are global store
writes from the student's own action: on a guest they go to the host as requests, so **a guest can advance or finish
the host's mystery** without the host seeing any of it; the guest's own town keeps its own mystery untouched.
**Protected now** (docs/COOP.md, "Variables each console keeps, and the host's alone"): g1000, g1001 and the other
mystery fields (those 0x020CE89C clears, those every mystery moves, those only events 071-122 write) are the host's
alone: a guest's writes are not sent, and once no event runs on the guest the host's values are back (verified with
pokes: a guest's g1001 = 3 stayed on the guest, not sent, then back to the host's; the host's g1000 = 4 and g1001 = 1
reached the guest). A guest's own code writing them (seen: g1000/g1001 changed on a guest by its own game while a
screen was open) is undone the same way.

**What the stages need** (the start records of events 071-122 that start real scenes, not set-ups; 488 records:
mc_ss 117, map_ss 221, map_u_ss 129, act_u_ss 21; per mystery in the decoder's `starts` listing) [V]: walking into a
town trigger area at a stage (the most common: e.g. mystery 1, stages 1-7 at areas 1-4; mystery 12, stage 1 at area 12
and stages 2-8 at area 17; mystery 24, stages 1-5 at area 26), entering a map at a stage (mystery 4: maps 47, 6, 8;
mystery 9: maps 63 and 48; mystery 31: the town with g1010 = 0), an actor or an incantation cast at the creature
(mystery 8 stages 3 and 6; mystery 30: t163 = 36; mystery 52: t160 = 3, t161 = 97; mystery 35: t24 = 3), extra
conditions on the mystery's own fields (g1002, g1003, g1010, g1023, g1024, g1039: what an earlier scene set), the
student's own fields (s1 = their dorm room 22-25 for scenes in the student's room, mysteries 18, 34, 41; s56/s54 for
mystery 37's haunted house), time windows (record +72, e.g. 600) and items (has_item in mysteries 5, 7, 11, 18, 37, 44, 46,
49). Many blocks start from other scenes or code, not from a record. Which mystery runs: `/script/start/obj.dat` (60
records of 12 bytes, one per mystery: a month and day range, or a place in a rotation of 38 kept in g1240), read at
the day roll (ov001 0x020C7AAC); a field load clears a g1000 the schedule does not hold.

**Built** (docs/COOP.md, "Mysteries together"): a mystery scene starting on one console (however it started) is
told to the other (message 43); the other student joins at once when free and near, else is asked "Go and help?"
and taken there (warp_local into the town, warp_map elsewhere, actor_set_pos beside, as events of ours); a busy
student is never interrupted and the scene never waits. Each runs the scene for its own student (rewards on each
record). The host decides the stage: a guest's own scene sends its stage writes to the host, which takes them if
the town is still at that stage (undoing its own joined copy); a joined guest's writes are dropped; a scene just
played does not start again at the same stage (`mq_myst_date` on the date check ov001 0x020F42C8).

**Co-op (the plan this followed).** Both join each stage: when a mystery record starts on one console, send its event (file, block) and the
trigger to the other (side channel), and run the same record there once its student is free (warped next to the
first if needed, as Lovers Point's meeting does in docs/COOP_RELATIONSHIPS.md). The host's console is the one whose
stage writes count; the guest's copies of those writes are dropped (not requests). Rewards written to the student
record (items, magic) happen on both, as each runs the script. Hooks: the start matcher's success (ov001 0x020F4A38's
return) or the script context's start (main 0x0201DAF0, the queue Connect now uses); a new listen type; a filter in
`mq_coop_sync_store` for the mystery's store fields (0-0x3F and the ones the scripts write). Battles inside mysteries
(057 Captain Dot, 150 the Gremlin, 999: query 0x67 -> ov019-021, `/script/mbattle.bin`) need their own pass. L.

## 2. Daily lessons

**What it is.** Each school day the student asks Principal Sol for a class (a menu of the lessons it has unlocked),
then sits with three classmates for a scripted lesson: linguistics, flatulence magic, incantations, tools (011, 021,
031, 041, 042; 17 + 13 + 23 + 9 + 3 = 65 lesson blocks), with questions put to the student ("Now, I think I'll ask
someone a question..."; right/wrong adds to s229, s227, s228), a note passed in class, the result counted per lesson
type (s241, s242, s243) and per house (g1520-1523, chosen by query 0x4A: the student's group of four [I]) [V].

**Where.** The menu is overlay 38 (`/talk/school/seluma.dat`), loaded from main 0x02014B54 / 0x020178DC; its pick is
written to **g1017** (town store index 0x11, ov038 0x0211C2B2) [V]. Entering the classroom (map 0) matches mc_ss block 0
records `g1017 = N` and runs that lesson [V]. The lesson's end writes s268 = 1, s365 = 1, s452, **g1015 = 1** and
**g1017 = 0**, house points g1520-1523, then warps to map 1 (the hall) [V]. g1015 = 1 is what starts the after-class
scene in the hall (mc_ss block 1 record 49 -> 010 block 10) [V]. Classmates in class are t131-t133, rolled per console
in ov003 (docs/COOP.md: guest rolls never sent). s210 (the school period [I], 0-3) is computed from the clock when a
building loads (ov003 0x020FF698) [V]: with host time on both, both agree.

**In a session before** (kept for the record). The staff are placed (`mq_coop_npc`), so lessons run; verified for
the first lesson. Each console ran its own; both consoles seat their own student at (3.5, 9.0), and the partner's
packets put it at that same place, inside the local student, so it looked absent (nothing hides it). The three
classmates were missing too (the building's placement, ov001 0x020C67F8 with field+0x3EAC = 0 in a session, sends
the scene's classmates home; docs/COOP.md). **Risk:** g1017 and g1015 are town variables written by the student's own action: a
guest's pick goes to the host's town as a request; if the host walks into the classroom first it gets the guest's
lesson, and either one's end (g1017 = 0) can clear the other's pick. g1520-1523 add up across players (wanted).
**Fixed (a):** g1015, g1017 and the after-class scene's sequence (g1253-1255, g1257, g1259; event 010 block 10 moves
them after each lesson) are kept per console; a guest arrives with its own town's values (verified: a guest's
g1017 = 7 from home given back on arrival, its g1017 = 5 / g1015 = 1 not sent, the host's g1017 = 2 not taken).

**Co-op.** Both join: (a) g1017 and g1015 stay on each console (never sent, never taken from the host: a filter in
the town sync for store indexes 0x0F and 0x11) - **built**; (b) the lesson starts when both are in the classroom: the
first one's start record is held (it reads g1017 as 0) while the other has a lesson picked; the second to arrive
starts at once, and the first goes into the classroom again (our event 90015, a warp as Sol's) so its record starts
it as the game does; each console decides from the other's state (side channel message 42), 90 s at most - **built**;
(c) the partner sits at the desk beside (6.5, 9.0), the classmate t131's: its packets from the classroom's seat
(3.5, 9.0) are shown there, and the placement leaves t131 out of the room - **built**; each player keeps its own
lesson when the picks differ (each its teacher and questions, same room, same time); (d) each student answers its own
questions; showing the partner's answer as a line - open. The classmates: in a session the game keeps people's
models 1-3 for the other players and puts every person in slot 4 (the teacher's), and kept none of the scene's
classmates; now two of them come in, in the slots of player objects no student uses (COOP.md) - **built**; with three
or four players there is one or no free slot.

## 3. School festivals and holidays

**What it is.** Four yearly school events, each announced by a teacher before class during the month before (start
records with calendar bytes +28/+29 .. +31/+32) and held the 2nd-8th of the next month [V]:
- 500 **spring tea party** (Miss Strawe; announced Apr 9-30, held May 2-8): host classmates in your room; graded.
- 501 **test of courage** (Mr. Vlad; Jul 9-31, Aug 2-8; also needs mystery 37's date).
- 502 **mushroom hotpot cook-off** (Mr. Oakley; Sep 9-30, Oct 2-8): gather mushrooms, judged.
- 503 **rainbow snow festival** (Principal Sol; Feb 9-29, Mar 2-8): collect snowflakes; bulletin board news (998:7016).
- 504 New Year's greeting (Dec 31 / Jan 1, Lovers Point, map 49), 505 New Year in the dorm (Jan 1-8), 110 Christmas
  (Santa, Dec 18-25), 101 Halloween (mystery 31).

They write the student record (s48, s52-s59, s508, s524-s546, s575) and done flags in the town (g1387-1390, g1242) [V].

**In a session today.** Per console, alone; a guest's done flag lands in the host's town.

**Co-op.** Both join the announcement (it is part of the lesson's start) and the judging scene; the gathering in
between is each its own (already works: items, mushrooms, snowflakes are the student's). The town done flags: host
decides (drop the guest's requests for g1387-1390). M for the announcement and judging together, L if the tea party's
hosting should accept the other player as a guest at the table.

## 4. Refused incantations

**Summons and transforms** (flag 212 test, then 999:3120) [V]: 332 World Tree, 350/351 Phantasm, 352 Candyman, 353
Werewolf, 355 Homunculus, 356 Familiar (writes g1310), 357 Golem (g1006 = 3), 359 Charm. (Dryad, 354, has no test.)
Each summons an actor or changes the student's model. Co-op: lift the test (`mq_coop_gate` matches only "goto, then a
bank 32 line"; these jump to a query 0xB7 and a sysmsg: a second pattern, or a list of the eight events' command
addresses); send the summoned actor or the transform to the other console (a side channel message naming the effect,
the other console spawns the same actor next to the remote student or swaps the remote's model). The town writes
(g1310, g1006) are the host's. M.

**Query 0xB5 refusals** (3114-3119, ov028 0x0211A16C) [V]: Connect doors (358), Sweet dreams (345), Make peace (321),
Sit by me (322), Honor student (346), Mushroom (349). Most look like a student's own effect [I]: Connect doors links doors (warps: the partner follows or not), Sweet
dreams plays a dream scene (queries 0x10/0x11/0x20), Make peace and Sit by me act on classmates, Honor student on the
student's standing, Mushroom on mushrooms (ground objects go through the host's grid). Co-op: answer 0xB5 as outside a session for these result ids (flag byte 199), one at a time,
each checked. S each, Connect doors M.

Rainbow, Flower rain, Rainmaking, Friendship and Declare love are the game's own two-player incantations (packet 0x45;
the host's slot writes the town's g1100/g1101) [V]; Friendship and Declare love are built between players
(docs/COOP_RELATIONSHIPS.md).

**Built** (game/src/coop_magic.cpp; docs/COOP.md, "Incantations in a session"): all of the above lifted except
Phantasm (its warp scene could not be verified), the caster's console runs each as alone, the end-of-spell cleanup
runs, the other console shows "<name> cast the <incantation> incantation!" with the cast's sparkle at the caster when
in the same area, and a guest's town writes (Golem, Familiar, Candyman; Mushroom's ordinary ones) reach the host,
which decides. Seen live: no summon leaves a creature or a changed model outside its own scene (only the cast, a
flash, the success line and those town variables), so nothing more is drawn remotely.

## 5. Evaluation and magic tournament

- **Evaluation** (590): Principal Sol, asked, says to come to the auditorium (590 block 2 sets s507 = 1); there
  (mc_ss block 8) he grades the year: titles such as Great Wizard, Evil Wizard, by mysteries solved, mischief, love
  confessions, trends (52:5353 ff.); then the staff roll (query 0xA0 -> ov025, `/title/stfrll_a.txt`) [V]. Co-op: both
  join; each is graded on its own record, called up one after the other. M.
- **Magic tournament** (520, 521, 530; started at map 8 with s225 = 3 or in town s225 = 2, s234 = 1): rounds against
  classmates; spells chosen in query 0xB1 (ov009 + ov034 `/picstat/magic`) [V]. Co-op: both join; when both entered,
  the first pairing is player against player (each console runs its side, the opponent's choice sent). L for a real
  duel, M for both in the hall each against classmates.

## 6. Gateway destinations

Event 250 block 1 [V]: in a session (query 0x22, t20) the Gateway offers only "communications room / Exit" (999:5316).
Alone it offers, by progress: **Anywhere** (5321, when query 0x96 = 1: a random warp to map 64, 67, 63, 9 the haunted
house, 53, or 6), **Mystery time** (5331, query 0x45 != 0: "Connects to mystery time"), **The other world** (5341,
mystery 32), and Save. Co-op: offer them in a session (the session test at #18-#19 made to fail, as `mq_coop_gate`
does for 509); host decides: when the host travels, the guest is asked and follows (warp next to the host). Anywhere's
own branch already tests the session (#207: map 6 instead of 67). M.

## 7. Insects, fish, mushrooms (and other map items)

Spawns are rolled on each console from the unshared RNG (docs/COOP.md, "What a session shares"): the partner sees
different insects and fish. Map items, weeds, ores, bones and berries are ground objects on the town grid (+0x438),
taken and put through packets 0x33-0x41 that the host arbitrates [V], and the day's map items are rolled by the host's
day roll: those already agree. Co-op: host decides spawns: the host's insect manager (field+0xA74C) and fish spawns
sent twice a second like the classmates (game/src/coop_npc.cpp); a guest's catch asks the host first (taken once). L.
Registering in the books is lifted (`mq_coop_gate`).

## 8. Classmates' life events

- **Transfers** [V]: g1061 (town) is rolled by the day roll (ov027 writes index 0x3D at 0x021161B6, 0x02116424 ...):
  1 = a classmate is leaving (550: a farewell at Lovers Point, then 0 or 2), 11 = homeroom announces the departure
  (551, then 12), 21 = homeroom introduces the new student (555, then 22). Homeroom runs when a student enters the
  classroom (mc_ss block 0 records 1-2). In a session it plays for whoever enters first, and its write (12, 22) goes to
  the host's town: the other never sees it. Co-op: both join homeroom (start it on both; host's write counts). M.
- **Classmates visiting your room** (400-402, maps 20-36, started by your relationship values s72-s79) [V]: the setup
  on entering your room is skipped in a session (254 blocks 60-63, flag 212). Co-op: each does own (lift; the guest's
  room is in the host's dorm, see 14). S-M.
- **After class** (010, 580-584 in the hall with s221 = 1; invitations "We'll go together"), **dates** with classmates
  (700-705, 291 with queries 0x44/0xC9 which read the session) [V]: per student, run today; a classmate taken by one
  student is busy for both (coop_npc owners). Verify only. S.

## 9. Small refusals

| Refusal | Where | Fix | Size |
|---|---|---|---|
| Storage chest = **the trash can** | 225 block 1: `if t212 == 1` (a guest only) -> 999:1021; query 0x54 opens ov017's "TRASH CAN" screen, which moves items from the bag (record +0x636) into a town list (+0x644A, 15 items) [V, tried] | Stays refused: lifted, a guest's thrown item stayed in its copy of the town list (never sent) and the host's thrown items could be taken out again (duplicates). Needs the list made the host's (requests) | S-M |
| School bell | 221 block 12 (8614): the bell is a chime: "Play music", "reset the bell", query 0x58 -> ov013 (`/chime/`) [V] | The melody is the school's (town [I]): host decides; let the guest play it but not reset it, or send the melody | S-M |
| Tatami room | 254 block 45 (5306): the purple door off the dorm hall, area 15, open when query 0x25's day is a multiple of 3 [V]; refused twice (query 0x22's t20 == 1, then t212 != 0) | **Lifted** (`mq_coop_gate`), verified on a guest | S |
| Bank | menu 5808 (deposit, withdraw, safe deposit -> 32:5860) instead of 5807; the selector is not in 214's scripts [V] | Find the selector (a `$017` text or the shopkeeper's talk), keep 5807: the money is the student's | S |
| Download | 32:8300 | Keep (Wi-Fi Connection) | - |

## 10. Town-changing magic and weather

Rainbow (310), Flower rain (313), Rainmaking (334): in a session the host's slot writes the town's g1100/g1101 (26, 29,
50) and each casting console sets its own sky (query 0x12, g1400 = 1); query 0x16 (ov028 0x02114E38 -> ov001
0x020FAF14) refreshes outside a session only [V]. The day's weather is a guest roll the town sync never sends (store
540-559); the host's bytes arrive but the guest's sky is drawn from what it read at load. Co-op: after a host weather
change (or the incantations) send "weather changed" and run the same refresh on the guest. S-M.

**Verified** (docs/COOP.md, "Incantations in a session"): the guest's Rainbow and the host's Rainmaking succeed on
both consoles with the same town bytes and weather shown; "The other person is not ready." is the game's own answer
while the other student is busy. The host now sends its weather bytes to the guests as soon as they change; the guest
draws them at its next area load (no immediate redraw found: ov001 0x020CFBC0 would also roll the guest's own).

## Other systems (working or small)

- **Shops** (Harrows 211 with CDs, the Emporium 212 with part-time work, Vivian's 213, the bank 214, Daisy's 215, the
  post office 216, the café 217, Mon Paris 218, 219): purchases, work and the café lifted and verified (docs/COOP.md);
  shopkeepers across the counter built. Open: stock and other town bytes written outside the store (COOP.md "Open").
- **Noodle stand** (227, Charlie's Noodles, 300 Ritch; also tells the berry price g1047): at map 64 or 66 by the town's
  g1516 (host's, rolled); no session test [V]. Works if the stand's NPC is placed (as the shopkeepers).
- **Outdoor areas 63, 64, 66** (town exits 0x9C, 0xAE, 0xA6; scratchpad doors research): door scripts 255 blocks
  57-78 have no session test; during mystery 31 stages 1-2 they answer "You are unable to enter" (33:1518) [V].
  Query 0x26 counts the *other* students already in the target area (t21) and calls a room full at 2 (the most
  rooms hold is 3 students), 4 for the communications room, and always for another player's dorm room (areas 22-25,
  t21 = 10) [V]: no limit for two players.
- **Haunted house** (map 9, rooms 10-13; doors in 254 blocks 31-48): no session test besides room capacity; mysteries
  075 and 107 (the tournament "bring back the item hidden in the haunted house") use it.
- **Letters and telegrams**: telegrams from the post office (500 Ritch, 216 block 2/5, keyboard ov009 + ov035), packet
  0x42 in a session; replies in bank 998. A telegram to the other player: docs/COOP_RELATIONSHIPS.md step 2 lists it.
- **Bulletin board** (ov016 `/board/`, news lines in bank 998 like the festival updates; school 221 query 0x7A):
  ov016 reads the session in four functions [V]; what for is not traced [I]. Verify it opens in a session.
- **Closet, uniform, design** (ov007 `/2d/mycloset/` reads the session [V], ov006): the student's own.
- **Hair salon** (ov014), **CD player** (ov011), **catalog** (ov012 `/catalog/uma_catalog.bin`), **creature catalog**:
  the student's own; no session test found.
- **Detention** (cleaning the toilets: 257, 801-804 on maps 68-69, "School Regulation, Item 99... Turning off the power
  without saving"): s500 = 1 starts it [V]. **Checked and handled** (docs/COOP.md, "Turning off without saving"): the
  load at the title sets s500 from a bit of the save's header that the visit's start clears and its end sets; a game
  closed after the end's save and before home was punished; a `<save>.visit` marker now forgives those loads.
- **First day**: built (COOP.md: gateway open, warden at his counter, the welcome for both, the first lesson).
- **Clubs, pupil council**: not in the game (no text, no scripts).

## The session's own code paths (where to hook)

Functions that read the session pointer (`*(*0x020ACA58 + 0xA28)`): main 61, ov001 44 (+60 that read field+0x3EAF),
ov000 3, ov003 1 (+8), ov007 1, ov009 3, ov016 4, ov024 2, ov028 14 [V]. The ov028 ones are script commands and queries:

| Query / command | ov028 | What it does in a session | Used by |
|---|---|---|---|
| op 0x02 actor_set_pos | 0x02111CE8 | does not re-centre the map on the actor | every scene |
| op 0x27 wait | 0x0211287C | shortens the wait by the frames the link fell behind | every scene |
| 0x0A | 0x021148D0 | an area reload, in step with the others (packets, waits for state 0x0E) | 080 (Mars greeting), 290, 999 |
| 0x16 | 0x02114E38 | the refresh after an incantation runs only outside a session | 311-361, 114 |
| 0x22 | 0x021152D0 | in session (t20) and slot | shops' keepers, Gateway, doors, two-player incantations |
| 0x26 | 0x02115550 | room capacity; another player's dorm room is full | doors 250-255 |
| 0x2F | 0x02115BD8 | answers 12 (none) in a session instead of scanning 12 store bits | not in events [I] |
| 0x44, 0xC9 | 0x021167C8, 0x0211A87C | classmate dating (701), with the others' records | 291, 999 |
| 0x4D | 0x02116970 | Secret saving | 360, 999 |
| 0x54 | 0x02116CA4 | the storage chest's screen; after it, ov001 0x020F14D4 | 225, 240 |
| 0xAE | 0x02119BEC (0x02119BA0) | packet 0x1A to the others at a scene's start | 001, 057, 071-122, 250, 253-255 |
| 0xB5 | 0x0211A16C | incantation allowed? refuses six, asks the other console for five | 310-361 |

Flag byte 212 tests (`if t212`) left today: 221 block 12 (bell), 225 block 1 (the trash can),
254 blocks 60-63 (classmates' room visits), 220 block 50 (a camera only), 332, 350-353, 355-357, 359 (summons) [V].
The menu tag `$016'A'B` (main 0x02038A9A): 5150 CD, 5353 work, 5780 seedlings lifted; 8300 Download kept [V].

## Suggested order

1. S items that make whole systems playable alone in a session: ~~g1017/g1015 local (lessons)~~, the trash can (a
   town list: requests), ~~the tatami room~~, the bank menu, ~~the six 0xB5 incantations one by one~~, classmates' room
   visits, ~~the s500 check~~ (done: docs/COOP.md).
2. "Both join" machinery (one generic message: start event file/block on the other console, with the partner placed
   by the script's player position): then mysteries' stages, homeroom, festivals' announcements and judging, the
   evaluation.
3. The partner inside scenes (seated in class, standing in a mystery's cutscene): the actor lookup returning the
   remote student for a classmate's or a free actor id.
4. Host-decided spawns (insects, fish) and weather refresh; Gateway destinations; ~~summons and transforms shown on both~~.
5. The tournament duel.
