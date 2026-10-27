# Testing: scenarios, perf logs and frame hashes

How to reproduce a performance or visual problem, measure it, and check that a change fixed it (or broke nothing else).
Everything here runs on your own machine with your own ROM and save: no game data is ever committed (docs/LEGAL.md).

## The idea

The emulation is **deterministic**: the same save, the same settings and the same input give the same game, frame for
frame (the DS clock starts at 2000-01-01 on every boot, and the program feeds the game its input once per emulated
frame). So a problem does not have to be described: you *record* the input that leads to it, once, and anyone with the
same save can replay it any number of times, faster than real time, with measurements switched on.

There are no savestates (the game's threads run on the host's fibers, docs/ARCHITECTURE.md), so a replay always starts at
the boot. That is cheap: unpaced, ten minutes of play replay in about 90 seconds.

## A scenario

`tools/scenario.py` keeps scenarios in `scenarios/` (git-ignored):

```
scenarios/NAME/start/      cfie.sav, cfie.wfc, settings.ini exactly as the recording began (never changed afterwards)
scenarios/NAME/rec.txt     the recorded input
scenarios/NAME/notes.md    yours
scenarios/NAME/results/    one folder per replay
scenarios/config.json      optional: {"exe": "...", "rom": "...", "data_dir": "..."}
```

Replays and recordings run in a scratch data folder made from `start/`; your real save and settings are never touched
(and the real game folder is linked, not copied).

### Record

```
python tools/scenario.py record water-lag --save path/to/cfie.sav
```

- Starts the game from a *copy* of that save (default: your real one) with your current settings, and records every frame.
- Play to the problem. **Press F12 where it shows** (as often as you like; a toast confirms). Marks tell the analysis
  which part of a long recording to look at. Then quit the game normally.
- A development build needs the ROM: `--rom "path/to/Echoes of Time.nds"` (or `rom` in `scenarios/config.json`).
  Otherwise `--data-dir` is the folder the setup wizard built the game in (default: the usual one).
- What the recording does *not* contain is what changes between machines: it holds the input, not the picture.

The recording is a text file (the format is described in `game/src/input_script.hpp`). It begins with comments that say
what it needs, and a replay refuses to run with other emulation settings (`--ignore-require` overrides):

```
# ffcc-recording 1
# rev 0.1.0 (e25b912)
# save cfie.sav 524288 bytes, crc32 1a2b3c4d
# require analog_movement=multiplayer aspect=0 cpu_speed=4 fast_loading=1 fast_saving=1 skip_logos=1
# mark 18240
10 press A 4
...
# warning: frame 9000: cpu_speed changed to 1: the replay drifts after this frame
```

Spell element picks (the right-stick radial or the number keys) are recorded as `spell N` (0-5, the gems top to bottom)
and replayed from it; a hand-written script can push the right stick with `rstick x,y [frames]` (percent, y down), which
opens the radial in screenshots too. The radial's sounds change the emulated memory like the menu's, so for state hashes
compare runs with the same setting.

The analog stick is recorded exactly (`analog x,y`), so a replay with a gamepad plugged in or a mouse moved gives the same
game: **a replay ignores the real keyboard, mouse and gamepad** (`--mix-real-input` adds them again). Changing an
emulation setting (CPU speed, fast loading, ...) in the menu while recording is noted as a `# warning:`.

### Replay and measure

```
python tools/scenario.py replay water-lag --label before          # unpaced, perf log and hashes
python tools/scenario.py replay water-lag --label after --exe other/ffcc.exe
python tools/scenario.py replay water-lag --shots-at marks        # screenshots around the marks
python tools/scenario.py replay water-lag --paced                 # in real time, to see it
python tools/scenario.py replay water-lag --display-hz 144 --extra --display-rate
```

A replay runs until a little past the last mark. Each result folder holds `perf.csv`, `hashes.txt`, `shots/`,
`input.txt` (the recording as replayed), `stderr.log`, `meta.json` (exe, git revision, options) and `summary.json`.

```
python tools/scenario.py summary water-lag           # the latest result
python tools/scenario.py compare water-lag:before water-lag:after
```

`summary`: percentiles (mean, p50, p95, p99, max) of the emulation time, the frame interval, the present time, the GPU
time and the 3D blending time, the slowest frames with their numbers, and the same for the window around each mark.
`compare`: whether the emulated memory is identical, the first frame where the memory and where the picture differ,
a difference image at the first differing screenshot, and the timings side by side.

## What the logs contain

`--perf-log <file.csv>` (with `--perf-from-frame <n>` to leave the boot out): one row per emulated frame (`E`) and one per
picture shown (`P`; display-rate 3D shows several per emulated frame, `k` counts them). Times in milliseconds.

| column | meaning |
|---|---|
| `frame`, `kind`, `k`, `t_ms` | emulated frame number, `E` or `P`, which picture of the frame, time since the log opened |
| `emu_ms` | `E`: host time spent emulating the frame (without the frontend's pacing and presenting) |
| `present_ms` | `P`: time `present()` took: drawing, the overlay, the swap; with vsync it includes waiting for the refresh |
| `interp_ms` | `P`: time to blend the 3D for this picture (display-rate 3D) |
| `gpu_ms` | `P`: GPU time of the drawing (timer queries read a few presents late; empty where the driver has none) |
| `late_ms` | `E`: how late the pacing let the frame start (paced runs). `P`: how late the picture came against its refresh or timer |
| `steps` | `E`: instructions the interpreter ran |

An unpaced run measures how much work a frame is, not whether it keeps up: compare `emu_ms` with a DS frame (16.7 ms). A
`--paced` run measures the hitches a player would see (`late_ms`, the `frame_interval`).

`--frame-hash <file>`: one line per emulated frame, `frame state_hash picture_hash`. `state_hash` covers main RAM, both
work RAMs and the CPU clock, so it changes with anything the game computes; `picture_hash` covers the window as presented
(every picture of the frame), so it also changes with rendering. The first differing line between two runs says where
they part ways: a memory difference is the game going another way (other settings, another build, a nondeterminism bug);
a picture-only difference is drawing (widescreen, HUD, interpolation, shaders). The same build twice must give identical
files; runs at different `--display-hz` must give identical `state_hash` columns.

`--shots <dir> <from> <to> <step>` saves the window as `frame_000120.png`, ...; `<frame> shot file.png` in a script and
`--screenshot-at-frame` stay.

## What breaks determinism

- A different save (`cfie.wfc` too: it holds the Wi-Fi friend code), a different ROM, or a different emulation setting
  (`# require` catches the setting ones).
- Another running instance (the wireless MAC depends on how many are running: run one at a time while you record and replay).
- Changing an emulation setting during a recording (noted as a warning); video settings only change the picture.
- A different window size changes `picture_hash` (it hashes the window), not the game.
- A different build of the program: the recording stays valid as long as the game logic is the same; comparing builds is
  what `compare` is for.

## By hand

The scenario tool is a thin wrapper; the options are all on `ffcc.exe` (`ffcc --help`, "testing"):

```
ffcc.exe <rom.nds> --data-dir run --input-script rec.txt --read-only-save --unpaced --exit-after-frames 20000 \
         --perf-log perf.csv --frame-hash hashes.txt
```

`--no-settings` ignores `settings.ini`; `--save-file <copy>` uses another save; `--set-at-frame <n> key=value` changes a
setting mid-run (not CPU speed or fast loading). A script is one step per line, see `game/src/input_script.hpp`;
`python tools/scenario.py new-from-script NAME --script s.txt --save cfie.sav` makes a scenario from one.

## Checks that run without a ROM

`ctest` runs `test_input_script` (a recording read back gives the input it was made from, bit for bit; marks, warnings and
the `# require` check) next to the other unit tests; `tools/check_no_game_data.py` keeps saves and screenshots out of commits.

## Testing efficiently (the session's harness)

The test machine is small (2 cores, 8 GB) and shared by several runs at once. Every test costs minutes, so pick the
cheapest one that answers the question and keep the machine clean. (The harness scripts live outside the repo, in
the session's /home/claude/run; the switches they use are in the game.)

### Cheapest first

1. Static reading: the disassembly and the event script decoder. No CPU.
2. One copy of the game (software renderer, unpaced, `NTR_DRAW_EVERY=-1`, no window presents between shots).
3. Two connected copies (instant connection: about a minute from boot to connected).
4. OpenGL runs (Mesa llvmpipe, 5-10 frames a second): only for what needs the layer split (the PC view).
5. The full regression: once per push to main.

### Switches that save time

- `NTR_DRAW_EVERY=<n>` (software renderer): the renderers draw 2 frames in n; the game runs the same. About twice as
  fast; pictures at most n frames old (0 for a fresh picture).
- `NTR_DRAW_EVERY=-1` (software renderer; the live harness's default since 2026-10-26): pictures **on demand**. Only
  the four frames before each screenshot are drawn (`--screenshot-at-frame`, `--shots`, a `shot` step of
  `--input-script` or `--input-live`; a live `shot` is put at least 5 frames ahead), so a shot is the picture an
  every-frame run takes: a frame's 3D scene is rendered at line 215 of the frame before, and in co-op's 3D on both
  screens the other screen is a display capture of the frame before. The first frame of a run of display captures is
  drawn too (the game shows a captured picture during some transitions long after: the regression's frame 2501);
  captures every frame (many scenes, and 3D on both screens) are not, so the picture such a run leaves behind may be
  stale. `-2` draws no captures at all. Checked: the regression's first 20500 frames with 38 shots (every 500 frames)
  are identical to `NTR_DRAW_EVERY=0`'s, and a connected pair's shots in town show both screens' 3D. The game runs the
  same (the regression's shots with the default are unchanged). Measured on one solo copy standing in the dorm (frames
  per CPU second, the machine shared): `NTR_DRAW_EVERY=10` with a present every 30 frames 140; on demand 170; on
  demand and no presents 230-244 (1.7x).
- `--present-every <n>`: the window is shown every n frames (a screenshot is always shown). Under Xvfb a present costs
  Mesa llvmpipe threads ~20% of a copy's CPU at n=30; the harness uses 1000000 (`PRESENT_EVERY`), as nothing looks at
  the window between shots.
- Sound: the harness runs with `SDL_AUDIO_DRIVER=dummy` (the audio thread uses ~0.1% CPU). The sound chip itself is
  emulated either way: the game reads its channels' state, so its mixing cannot be skipped without changing the game.
- `<file>.frame` of `--input-live` is one file kept open and written over (a reader never sees it empty), and
  `MQ_POS_FILE` is rewritten only when the place changes: opening and renaming files every 6 frames showed in ~4 of
  30 stack samples of a copy on the test machine's file system (measured gain ~3%).
- `MQ_COOP_AUTO=host|join` (or F9, or Settings > Network > Connect automatically): a co-op visit at once, no gateway,
  no menus. `NTR_LINK_NAME=<word>` gives a set of test copies their own link, so pairs don't meet each other.
- `MQ_WARP_FILE=<path>`: write `e <arrival point>` (outside the café 235, books 243, Harrows 233, school 240, dorm
  221; a building's inside is its door's number + 1) or `<map> <x> <z>` and the player is sent there. Close any open
  talk or menu first: it carries over a warp. The town's map is 255 (map 0 is the classroom). In a session a
  `<map> <x> <z>` warp into the map the console already stands in is refused (it once ended the game: the field
  reloaded under the touch handling, prefetch abort in ov002 0x02102564); use `e <entry>` there.
- `MQ_MYST_GOTO=<path>` (co-op mysteries): `<map> <x> <z> <y>` or `<arrival point> 0 0` written there sends the
  student there with an event of ours (warp_map / warp_local), as a join does. `p <x> <z>`: a step within the map the student
  stands in (actor_set_pos, event 90018). In a session, map warps into the town
  made by `MQ_WARP_FILE` (`255 x z`) end the game (fatal at ov002 0x02102565), also from the dorm or Lovers Point,
  and so does warp_map 255 in an event: reach a place in town by an arrival point (`e 240`, or `240 0 0` here).
  A mystery to test: poke g1000/g1001 on the host *after* standing in town (the next map load clears a mystery the
  schedule does not hold), queue the mystery's set-up block on both (e.g. 8232, 12130), then its scene
  (`MQ_EVENT_FILE`); `MQ_MYST_LOG=1` logs every event start. Harness: `/home/claude/run/live/mysgo.sh`, `myst.py`.
- `MQ_EVENT_FILE`, `MQ_COOP_POKE_FILE`, `MQ_COOP_CAST_FILE`: start one of our events, write a variable, cast an
  incantation, instead of playing up to it (the cast file takes a result id: 30 and 32 are cast with the other
  student, any other through the game's own cast, main 0x02047A10, docs/COOP.md "Incantations in a session"; the
  test needs the student free in the field; it skips the magic menu, so Phantasm, which the menu's place test refuses
  in the town and the dorm, is refused there too unless `MQ_COOP_CAST_FORCE=1`; its place is the dorm's washrooms, maps
  4 and 5: `MQ_WARP_FILE` `4 5 4`). Relationship tests start from high
  values in the `.coop` file.
  `MQ_MAGIC_LOG=1` logs the incantation events' tests and what coop_magic does. The real input (transform, words,
  item; `MQ_CAST_LOG=1`): "Casting an incantation by real input" below.
- The Gateway's destinations (docs/COOP.md, "The Gateway's destinations together"): `MQ_GATE_ANYWHERE=<file>` sets the
  day's Anywhere flag (g1262 and its copies) while the file exists; `MQ_GATE_KEY=<file>` lets the other world's item
  test pass while it exists; the other world itself: poke g1000 = 32 and g1010 = 1 on the host after connecting.
  day1done.sav's students have s411 set (the principal hasn't told them about the Gateway: only the room and Save):
  poke `var 411=0` on the copy that will touch it, before or after connecting (`MQ_COOP_POKE_FILE`'s `var` writes
  the local student's record). Open the Gateway with
  `MQ_EVENT_FILE` = 25001 only where the student stands at it (upstairs in the dorm, where Connect now leaves both):
  its walk into the Gateway is relative, and run elsewhere (once, on the beach) it walked the student into the beach's
  exit and pushed the other student out too. Harness: `/home/claude/run/live/gwpair.sh` (`PRE_POKE`, `GW_ENV`, each
  copy with its own `MQ_MYST_GOTO` file) and `gwtest.py <user> <partner> <pick> <yes|no|none> <tag>`.
- Fishing by pad (docs/COOP.md, "Fish together"): R twice takes the rod, X held about 100 frames casts to the
  river's middle from its west bank by the bookshop (`e 243`, then the student at about (40.2, 73) facing east).
  `MQ_FISH_TRACE=1` logs the lure, the hooked fish's state (5: the bite) and the rod's fight count; a driver must not
  press X while the fish pulls (count below 100: those presses count toward its escape) and taps X after it until the
  lure is in. Harness: `/home/claude/run/live/fishdrv.py <copy>` (outside the repo).
- `--screenshot-at-frame <n> <file>` can be given several times in one run.
- **Who writes a piece of RAM:** `MQ_WATCH=<hex address>` logs every write to that word with the guest function last
  entered and the ones before it (writes to its page take the slow path; only that page). `MQ_WATCH_LEN=<bytes>`
  watches a range from the address instead (each of its pages), and with `MQ_WATCH_STRIDE=<bytes>` and
  `MQ_WATCH_KEEP=<lo>-<hi>[,...]` only those offsets within each stride, e.g. the classmate records' needs:
  `MQ_WATCH=020B5488 MQ_WATCH_LEN=0x854 MQ_WATCH_STRIDE=0xA4 MQ_WATCH_KEEP=0x48-0x50,0x9F-0xA0` (a writer of ours,
  the town sync's native writes, shows as whatever guest function was entered last). `MQ_WATCH_FROM` / `MQ_WATCH_TO`
  limit it to frames.
- `MQ_POS_FILE=<path>` holds **the student's place** ("x y z" in units, every 6 frames): the field's local player
  object, `*0x020AD7EC + 0x383C`, x, y, z at +0x2C, +0x30, +0x34 (field+0x3868: what the talk finder ov001 0x020C85E4
  measures from, what the game's movement packets carry, and the frame the event scripts' places are in: warp_map
  64 (5, 14) lands at (5, 14)); `game/src/mq_student.hpp`. While a map loads the object reads (0, 0, 0) for some
  100-150 frames: the file keeps the last place then. Before 2026-10-26 the file held 0x020B8B7C, which is **the
  bottom camera's point** (field+0x168, camera 1's +4), not the student: in town 3.4 south and 8.7 up at the school's
  front and the dorm's square, about 7 south and 4.7 up at the plaza; 8.22 south in the dorm, 11.85 on the beach,
  12.23 at Lovers Point; and it keeps the old place for a while after a map change. `MQ_POS_CAMERA=1` writes the
  camera's point instead (old notes and positions in docs/COOP.md written before that date are camera points);
  `MQ_POS_TRAIL=1` appends "frame x y z cx cy cz" (student, camera) to `<path>.trail` every 30 frames. Co-op code
  reads the student the same way (coop_myst, coop_gate, coop_hall). `drive.goto` learns its directions from motion,
  so it works on either.
- day1done.sav loaded alone is punished at its load (the warden's "Did you turn off the power without saving?!",
  then his scholarship talk on a new month), and an open talk carries over a warp and keeps the town's set-up from
  running on that load: put an empty `<save without .sav>.visit` beside the copied save before the copy starts (the
  visit marker, docs/COOP.md "Turning off without saving"; `start.sh` copies the save to `s.sav`, so `s.visit`),
  and press A through the dorm's talks until the student walks before poking or warping.

### A second test save: a student of its own

Every earlier test save (day1done.sav, explore.sav, game.sav) holds one student: AAAAAA, the same identity (the 8
bytes at the record's +0x0C, save offset 0x996C: `28f966823fcec90a`), and two copies of one save make a "pair" of
one student. `/home/claude/run/saves2/second_day1.sav` (+ `second_day1.wfc`, notes in `second_day1.notes.txt`,
outside the repo) is another: **NORA**, female, birthday 14/7, school WILLOW, 3000 Ritch, identity
`28f966ba7f3a30cf` (pair key `4e4f5241-28f966ba7f3a30cf`), her own friend code file. Like every save's first student
she is slot 0 of her own town (record +2), as AAAAAA is. Use it as the guest (`GSAV=` / `GWFC=` of
`/home/claude/run/live/tgpair.sh`) or the host (its 4th argument) with day1done.sav as the other.
How it was made (a solo copy, by pad and touch; `tgsolo.sh <name> -` starts one without a save, helpers in `tg.py`):
1. New game, `--clock 2000-01-01T00:05:00` (scripted runs start at 2000-01-01 too; a live copy without `--clock`
   takes the PC's date): touch the title (128, 150), *Enroll* (128, 74), A through the old man. Each A also types
   into a keyboard that is open (likely how the old saves got "AAAAAA"), so press A one box at a time and look.
2. The DS keyboard by touch: key (row r, column c) at (23 + 21c, 64 + 16.3r) (A-F / G-L / M-R / S-X / Y Z rows),
   DELETE (201, 145), OK (193, 178); the number pad: 7 8 9 at y 64, 4 5 6 at 81, 1 2 3 at 96, x 171 / 192 / 212.
   Name, birth month, day; the gender menu (Male / Female, Down for Female); the appearance ("Let me pick. /
   Exactly! / Not quite...": Exactly! takes the old man's pick); the school's name (the town's: typed the same way);
   "No mistakes." The game saves (`nora_enrolled.sav`, the state regs/base.sav has for AAAAAA).
3. From that save (title, then *Continue* at (128, 55)): the warden's intro by A, walk Down past his desk ((15.7,
   10.5): "Hey! HEY") for the walk to school, the welcome and the class by A; out of the classroom by its left side
   (x 0, z 10.4), through the hall onto the mat (x 11, z 20) into town; `e 221` and Up + A into the dorm (Reginald's
   scholarship scene, 3000 Ritch); the stairs at (19.8, 6.6) going Up; upstairs the Gateway at (14.5, 2.6) facing
   Up, A: "communications room / Save / Exit", *Save*, Yes: "Save complete.". A real save: it loads without the
   turned-off-without-saving punishment (day1done.sav has that bit; the harness's `.visit` marker forgives it).
   (`e 222` is not the dorm's inside: the warp file read it as map 222, a sky over the sea.)

### Talking to a festival's teacher or Mr. Graves for real

Pressing A by Miss Strawe at the tea party and by Mr. Graves on the 21st works in single player, as the game has it
(verified 2026-10-26: the tea party from her first talk to the table scene, and Mr. Graves' menu, "Extracurricular
lesson", the gathering in town and the auditorium's rules, all by pad). Earlier tests that found "no talk" and "Mr.
Graves not drawn" stood the student by the pos file when it held the camera's point (see `MQ_POS_FILE` above), about 7
units north of the teacher, out of reach and with the teacher below the bottom screen. What a real talk needs:
- **The day.** The tea party's set-up (500.2, mc_ss block 100 record 53) runs on May 2-8 (`--clock
  2000-05-03T15:30`); the test of courage (501.2) August 2-8, the rainbow snow festival (503.2) March 2-8. Mr. Graves
  (510.1, record 52) stands at (62, 78) with his blackboard every day before the 21st (his own talk, no tournament)
  and from the 21st only for a student who knows four of the tournament incantations s296-s307 (`--clock
  2000-05-21T15:30`, poke `var 296=1` ... `var 299=1` if the save knows fewer).
- **The set-ups' state.** Records 53-55 want s411 = 0 and the load's byte +7 = 0; record 52 wants +7 = 1 (the
  field's +0x3EB0, copied from 0x020AD7FE, made 1 at the day's start only when s411 is clear). day1done.sav has s411
  set: poke `var 411=0` for a festival, and for Mr. Graves also `20ad7fe=1`, both before the town loads.
- **A town load after that**, with no talk open: `e 239` (the post office, by the plaza) or any door. Then check a
  RAM dump: the teacher is in the field's people list (`*0x020AD7EC + 0x3E48`, 0x2B0 each, count +0x3EAD, id +0xA1:
  124 Miss Strawe at (62, 74), 125 Mr. Graves at (62, 78)) with a model.
- **Stand in reach, facing them.** The finder wants the teacher within reach in front of the student at the same
  height. Both stand facing south; the student about 1-1.5 units north of them, pad Down to face them, A. Seen:
  the student at (62.57, 72.95) for Miss Strawe (the camera's point then 62.27, 79.95), at (62.59, 76.96) for Mr.
  Graves (camera 62.26, 83.94). The pos file now holds the student's place, so `drive.goto` to 1-1.5 units north of
  the teacher stands the student there. Miss Strawe: "Pleased to meet you, ..." on a first talk, then her menu and the party; Mr. Graves:
  "Nice to meet you, ...", "Oh, what's going on there?" and Chat / Extracurricular lesson / Quit.
Harness (outside the repo): `/home/claude/run/live/tqsolo.sh <name> [clock]` (one copy with the marker, poke, event
and goto files) and `tqlib.py` (`free`, `poke`, `warp`, `ram`).

### Casting an incantation by real input

How a player casts (2026-10-27; static: the charm table, main 0x02046E90-0x02047A6C, ov001 0x020CAC64; seen with
`MQ_CAST_LOG=1` and gdb entry breakpoints on the cast's checks). Each incantation's lesson tells the steps ("First,
you'll need an emerald... the two of you must go together to the ruins... transform into a wizard and chant the magic
linguistic spell. Hold the emerald in your hand..."), and the charm record holds them (`/magic/magic_charm_comp.bin`,
92 bytes: +0 the actions still wanted after the words, +18 the words, +54 the item, +70/+71 the place, +88 the target):
1. **Be where and when the record wants**, with its item in the bag. Friendship (record 4): an emerald (item 428), the
   town (+70 = 0), ground zone 32 (+71) = **the ruins**, the town's west block x 0, z 4 (about x 0-16, z 64-80; the
   zone grid is field+0x8F3, 5 x 6 blocks of 16 units, main 0x02047650 / ov001 0x020D5650). Way from the school's
   arrival point (e 240): south to (46, 70), round the slope to (47, 78), west along z 78 to x 18, then north-west to
   (13, 73) (`castlib.walk_to_ruins`).
2. **Transform into a wizard**: turn the D-pad clockwise (Right, Down, Left, Up, 6 frames each, four turns; the stylus
   circling clockwise too). This is **flag 510**: action 30 (main 0x02025508) writes it at 0x02025620, the hand gets
   the staff (3949), the tip "Transform into a wizard! Use magic linguistics to perform incantations." waits for A (step
   7), then the words screen opens by itself. Turning counter-clockwise changes back (510 = 0). A map change also ends
   it. (R cycles the hand's tools: 4081 the staff, 660-663 net, rod, ...)
3. **Say the words** in that screen (or Y) and OK. Friendship is three words: *incantation* (the hat, BASIC row 0
   col 2, id 3), *all the time* (the infinity sign, BASIC row 0 col 4, id 145), *together* (DO, row 0 col 1, id 77);
   ids by `/picstat/magic/order.dat`, names by `msg_e.dat` (index id-1). Every incantation begins with *incantation*;
   without it the words match the catch-all record (event 382, "nothing happens"). In wizard form OK chants (main
   0x02046F98 -> 0x02046B5C): the words drawn above the student, action 23, the incantation's 90 s at 0x020B8E26
   (5400 frames), player+0xC1 = 1; the record found then wants its action (0x020476F4 answers 2) and waits.
4. **Hold the item**: touch the bag (bottom screen, upper left), drag the emerald out of the bag onto the open field
   (not onto the student or a person: dropped there it is put on the ground, action 26). The student holds it up
   (action 22); 40 frames later the hold is the cast (main 0x02046E90 with kind 14 and the item): the record's other
   tests (main 0x020478A8) and its event, 320 block 1. Holding with X (kind 1) and putting it down (kind 15) are the
   other actions some records want.
5. **The target**: Friendship's +88 = 1 wants a classmate going about **with** the student (*0x020B8EF4, set by query
   0x37 when a classmate agrees to hang out: "Let's play!", event 705 block 3, or the classmate's own "You wanna do
   something together?"); without one the cast fails ("You failed at using your incantation.", event 320 block 2:
   main 0x020474F0 refuses it). **In a session** that test is skipped and query 0xB5 asks the other console (packet
   0x45): a student there who has **transformed** joins (action 27, the game's own event 320 block 4 "You were only
   slightly excited."), and coop_talk adds the pair's friendship ("Your Friendship incantation reached <name>!" /
   "<name> cast the Friendship incantation with you!"). Both holding the emerald at once fails on both (each is in its
   own event when the other asks): **one holds, the other only transforms (and may say the words)**.

`MQ_CAST_LOG=1` logs every change of the local student's action and step, the chant flag and timer, the queued cast,
the hand, flag 510, the chanted words' count, the running event and t199. Harness (outside the repo):
`/home/claude/run/live/castsolo.sh <name>` (day1done copy, own binary `castbin/mqcast`, `MQ_VAR_WATCH=510`),
`castpair.sh` (a connected pair, own link), `castlib.py` (`spin_right`, `spin_left`, `friendship_words`, `open_bag`,
`hold_from_bag`, `walk_to_ruins`, `talk_to`), `castfr2.py <copy> [tip] [hold]`, `castpair_fr.py`. Emerald: no test
save has one: poke it into a free bag slot (`20ADE5C=428/2`, slot 1; with the bag closed, or the bag's own copy
writes it back). NPC positions in a RAM dump lag; classmates wander, so reaching one for the hang-out is slow
(`castlib.talk_to`).

### More real-input recipes (sweep part 2, 2026-10-27)

Harness outside the repo: `/home/claude/run/live/sw2pair.sh [clock] [host] [guest] [save]` (own binary copy, own link,
`GSAV` for a different guest save, `H_ENV`/`G_ENV`), `sw2lib.py`, `sw2_words.py`, `sw2_ins.py`, `sw2_insgo0.sh`.
- **The magic words screen** (Y in wizard form, the tournament's 0xB1): six type tabs at y 178 (x 13, 37, 61, 84, 107,
  130: BASIC, WHO, DO, WHAT, WHERE, WHEN), 3 rows x 5 tiles per page (tile (r, c) at x 48 + 40.6 c, y 39 + 40 r; a
  tile is double-tapped), the next page's arrow at (246, 94), OK at (208, 178). A tab's words follow
  `/picstat/magic/order.dat` from its first index (BASIC 0, WHO 30, DO 165, WHAT 195, WHERE 240, WHEN 285). The first
  touch on a screen that has just opened is lost: wait about 40 frames. The charm table is LZ10
  (`/magic/magic_charm_comp.bin`, 43 records): +18 the words then the event (ids over 300), e.g. the tournament's twelve
  are records 20-31, "magic" + two words (party popper: *flower*, *instrument* on WHAT page 1; sleep: *daytime* on
  WHEN, *sleepy*), Rainmaking is record 17 (*incantation*, *sun* on WHAT page 3, *sad*, item 426).
- **Answering a co-op question** ("Go along with <name>?", "Go with <name>?"): A through its first box, then the
  Yes/No menu takes A only about a second after it opens (a press right away is lost); the answer must come within
  a minute of the question.
- **Insects**: the net (R until the hand is item 662); X swings; with the net in hand a touch on the bottom screen swings
  too (a touch on an insect walks there first). Catch: stand 1 unit from the insect's place (+0xFC, not +0x114, which
  is only where it is drawn) facing it, X. Ground butterflies by the bookshop are rare: a fresh pair per clock until
  one sits there (the clock seeds the rolls).
- **Doors**: walk into the door's rect from outside, then A (the bookshop from (32.5, 75)); the post office from
  (63, 60.5) by `e 239`; Thom at the counter from (5.5, 5.8) facing Up.
- **The other world**: on the host `var 1000=32`, `var 1001=2`, `var 1010=1`, `var 1011=0` and item 2106 in the bag
  (`20ADE5C=2106/2`), A at the Gateway (upstairs in the dorm), Down, A; the tower's door is at (5.4, 6.2) (walk Up
  into it, A); the old man (actor 212) at (11.9, 4.8).
- Stop scripts: `stop.sh` kills by a pattern of the save path (`live/<name>/s.sav`): a shell whose own command line
  holds that text is killed too (exit code 144); stop copies by PID or `pgrep -x MagiciansQuest` and their
  `/proc/<pid>/exe` instead in such commands.
- A one-shot gdb watchpoint finds who writes a guest address: break once on a recompiled function, read the host
  pointer of the address from the CPU's `read_pages`/`write_pages` (NtrCpu +104 / +112, one pointer per 4 KiB page),
  `watch` it with a Python condition, delete the breakpoint (sweep part 2's `wfc.py` in the session's scratch).

### Co-op between PCs on one machine (UDP on loopback)

The same-PC link (shared memory) is what most co-op tests use; the PC-to-PC link is tested by giving the two copies
`--lan-host <port>` and `--lan-join 127.0.0.1:<port>` (or `--lan-join auto:<port>`: discovery by broadcast, which
reaches the host on the same machine too). With a LAN mode set a copy uses UDP only, so `NTR_LINK_NAME` plays no
part: pick a port of your own (not 49251) so other pairs on the machine don't meet yours.

- Connect: `MQ_COOP_AUTO=host` / `join` on the copies (Connect now once free to move), or `MQ_COOP_CONNECT_FILE=<path>`
  and write `host` / `join` there later (e.g. after warping both into town, to test the walk back to the dorm).
  The harness scripts `udppair.sh <host> <guest> <save> <clock> [port]` (`AUTO=1`, `JOIN_ADDR=auto`, `BIN`,
  `H_ENV`/`G_ENV`) and `udpgo.sh` (the `fastgo.sh` flow) in /home/claude/run/live do this; `fastgo.py <host> <guest>`
  then reads the intro and waits until both stand in the host's dorm.
- Check the side channel and the town sync: `MQ_COOP_POKE_FILE` (`var <n>=<value>` on each) and RAM dumps of both
  taken together (`MQ_RAM_DUMP`, touch `<path>.req` on both; compare the town block, 0x993C bytes at 0x020AF020:
  only the checksum, the guest's time offset at +0x2D4 and the clock bytes may differ). The log says
  `lan: joining`, `lan: PC <id> is here`, `coop sync: node <id> is slot n` once the side channel runs.
- A bad network (testing switches of `runtime/src/lan_wireless.cpp`, on either copy or both):
  `NTR_LAN_LOSS=<percent>[:coop]` drops that share of the datagrams sent (every kind, or with `:coop` only the side
  channel's, which then also limits the delay to them), `NTR_LAN_DELAY=<ms>[:<jitter ms>]` holds each (the jitter
  reorders them), `NTR_LAN_WAIT_MS=<ms>` waits longer than
  25 ms for the guests' replies. The log counts the drops every 10 s. Expect the game's own exchange to fail above
  ~5% loss on its frames or a round trip above ~25 ms (docs/COOP.md, "What is built"); `NTR_LAN_LOSS=10:coop`
  tests the side channel alone.
- A guest that leaves: stop its copy (the host says "Unable to receive data" after 5 s), start it again from its
  save (`cp <guest>/s.sav` first: the start script copies the save it is given) and connect both again.
- Clean the RAM dumps of an earlier run first (`ram.<n>` numbering starts at 0 in each run).

### Co-op over the internet on one machine (four processes)

`--coop-internet` with `--lan-host` / `--lan-join` (docs/COOP_INTERNET.md): each player's copy starts its own hidden
stand-in (its log: `<save folder>/coop-standin/standin.log`), so a pair is four processes: stop other runs first and
keep it short. `netpair.sh <host> <guest> [clock] [port]` (`NETENV="NTR_LAN_DELAY=75:20 NTR_LAN_LOSS=5"` for both
copies, `HSAV` / `GSAV`, `BIN`, default a copy of the build in /home/claude/run/net so a rebuild does not touch a
running test) and `netgo.sh` (then `fastgo.py`) in /home/claude/run/live. `stop.sh <host> <guest>` stops the
stand-ins too (their command lines hold the player's folder). The logs say `coop net: both in the session` once the
far player's packets are swapped in on that copy; `MQ_NET_LOG=1` logs every packet tapped, swallowed and put in, and
`MQ_NET_PASS=1` keeps the stand-in's packets (no swap: research). The title's status is logged as `coop net: the
window's title says: ...`.
Real input over the internet with two different students, a bad network and a driver per feature (2026-10-27):
`inetpair.sh [clock] [host] [guest]` (host `day1done.sav`, guest `second_day1.sav` with its `.wfc`, visit markers,
Connect now, `NETENV`, own binary copy `/home/claude/run/inet/mqinet`, `ARB=1` for the arbitration log) and
`inet_all.sh` / `inet_rest.sh` (chat, gift, the Gateway and the tower, homeroom and a lesson, a mystery, Friendship,
a telegram and the cat) with the `inet*.py` wrappers that point the earlier drivers at the pair. A map warp of a
student (`MQ_WARP_FILE`, `MQ_MYST_GOTO`) is fine over the internet; the two hangs seen after one were the telegram's
stuck inbox (docs/COOP_INTERNET.md).

### The co-op soak: one long session, features combined (2026-10-27)

What breaks only when features meet over a long session (docs/COOP_PLAYTEST.md, "Soak"). Harness outside the repo,
in /home/claude/run/live:
- `soakpair.sh [clock] [host save] [guest save]`: the internet-mode pair `soh` (host, `day1done.sav`, AAAAAA) and
  `sog` (guest, `second_day1.sav`, NORA), own binary `/home/claude/run/soak/mqsoak` (`MQ_BIN` for another), own port
  (`PORT`, default 49871), `NETENV` (default the 200 ms test: `NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`), visit markers,
  Connect now, the poke / event / goto / cast files, the fish, insect, mystery and cast logs. `KEEP=1` starts the
  next session of the same players from the folders' own saves (keeps `s.coop` and the guest's `s.wfc`).
- `soak.py <day> [seed] [step,step,...]`: the scripted sequence by pad and touch, with small random variations from
  the seed (who goes first, which student walks into the mystery, the routes' points within a unit). Day 1 (connect
  at 14:00): `town` (out of the dorm, walk together to the school's square), `apart` (the host to the bookshop's
  side, the guest to the school's front: two districts, then back), `chat` (both ways), `gift` (both ways), `shop`
  (both into the bookshop, the clerk, out), `fish` (both, `fishdrv`), `insects` (the net, the nearest insect),
  `friendship` (the ruins, `castpair_fr.py`), `school` (homeroom and a lesson, `inet_hr2.py`, `inet_les.py`),
  `mystery` (the walk-in at area 10, `w6walk.py`), `leave` (the host's Disconnect: both save; copies of the saves in
  /home/claude/run/soak/saves). Day 2 (`KEEP=1 soakpair.sh <clock> <the day-1 saves>`, the new day turns in the
  session): `newday` (waits for the host's Disconnect and the reconnect), `walk2`, `door` (both
  through the school's door by pad; at night it is shut), `gateway` (the dorm upstairs, the other world, the tower,
  back), `lovers` (Lovers Point, a chat there, back to town). **The day's start is the season's**: the town's periods
  table (field+0x3E80, the first byte of each season's 8) starts the day at half hour 8 in spring, 6 in summer (3:00),
  7 in autumn, 9 in winter; a pair connected after that hour has turned the day alone at the load, so for the turn
  in the session start before it (summer: `KEEP=1 soakpair.sh 2000-07-19T02:55:00 <host day-1 save> <guest
  day-1 save>`). While waiting, press nothing: A by the Gateway opens its menu, A facing the other starts a talk.
- After every step, and every 7200 host frames (two in-game minutes), both consoles' RAM is dumped together and
  compared: the town block (0x993C at 0x020AF020) without the bytes each console keeps (coop_sync.cpp: the
  checksum, the time object +0x2C8..+0x2D7, the classmates' values toward the player slots, kOwnVars), the host's
  variables (kHostVars) apart, each student's place on its own console against the other console's copy of it
  (field+0x3E4C, when both stand in one area and the student stands still), the clock (+0x2CC/+0x2CD, over 2
  minutes apart), the date, the sky kind, the `.coop` pair records. A difference is read again 4 s later and only
  then logged as `DIVERGENCE ... after step <the step before>`. A monitor thread logs frame stalls over 3 s
  (`STALL`), frame-rate drops below 40 % of the median (`FPS DROP`), fatal lines, the logs' connection events and
  the copies' processes ending. Log `/home/claude/run/soak/logs/soak<day>.log`, pair shots
  `/home/claude/run/live/shots/soak/d<day>_*.png`. All RAM readers of the libraries go through one lock in the
  driver (each deletes the dumps and asks for a new one); steps that run other drivers as subprocesses pause the
  periodic checks. A classmate's needs (+0x48..+0x4F) that differ are a divergence again since they are kept in step
  (docs/COOP.md, "The classmates' needs"; `SOAK_NEEDS_NOTED=1` logs them as `noted` apart, as before). `soak_here.py`
  samples the host's copy of the guest while the guest wanders; `/home/claude/run/soak/room_talk.py` replays the
  Gateway + talk case of docs/COOP_PLAYTEST.md "Soak".
- Lessons for drivers: the routes by the river ((43, 55) -> (37, 64)) stick at (39, 58); `soak.py` warps to the
  bookshop (`e 243`) when a route fails. A by a door or a counter with the other student in front starts our talk
  (`enter_books` steps the other aside and answers Never mind). `dllib.free` presses B, which does not close our
  talk's menu: `dllib.nevermind` does.
- A private build for a fix while others edit the shared tree: `/home/claude/run/soak/build_soak.sh` (a `cp -a`
  copy at /home/claude/soaktree built in its own mount namespace, the `build_tg.sh` recipe).

### Regression tiers

- Quick: the first 20,000 frames (the dorm, the school, the principal's welcome), about 2.5 minutes, both screenshots
  pixel-identical to the baseline. For changes that only act in co-op or the PC view.
- Full: 80,000 frames, about 11 minutes, eight screenshots pixel-identical to the baseline. Once per push to main,
  batching verified features; built from the exact commit in a clean worktree, so nobody's unfinished edits are in
  the tested binary, and run through a renamed link to the binary, so kill patterns aimed at other runs can't stop it.

### Don'ts (each cost hours once)

- A debugger breakpoint left on code that runs every frame (Python or conditional) slows the game about ten times.
  Use gdb only to read or write once and detach; put research logic in an env-gated branch of a hook instead.
- Killing every game copy by name stops other people's runs (regressions died this way). Stop your own copies by a
  pattern unique to your save path, in a separate command from any launch.
- A wait loop that `pgrep -f`s a pattern that appears in its own command line never ends.
- Killed runs leave their virtual displays (`Xvfb`, about 67 MB each) behind: clean the ones whose parent is gone and
  that are over an hour old.
- More than two or three game-running jobs at once on two cores only slows all of them. Agents that only read and
  write code are cheap.
- Two builds at once in the same build folder break each other (missing object files). Build the shared tree with
  the session's `/home/claude/run/build.sh` (a lock: a second build waits for the first). Checking `pgrep -x cmake`
  first still races (two agents started in the same second); a `ps | grep -c "[c]make --build"` count includes the
  checking shell's own command line and never reads 0.
- `git commit -a` in a working tree other agents are editing commits their unfinished work too: commit explicit
  paths (`git add <file>`; for a shared file, your hunk alone via `git apply --cached`).
- Reading a screenshot quickly: a camera pointing at the sky was once taken for a working feature and shipped.
