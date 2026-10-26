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
2. One copy of the game (software renderer, unpaced, `NTR_DRAW_EVERY=10`).
3. Two connected copies (instant connection: about a minute from boot to connected).
4. OpenGL runs (Mesa llvmpipe, 5-10 frames a second): only for what needs the layer split (the PC view).
5. The full regression: once per push to main.

### Switches that save time

- `NTR_DRAW_EVERY=<n>` (software renderer): the renderers draw 2 frames in n; the game runs the same. About twice as
  fast; pictures at most n frames old (0 for a fresh picture).
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
  in the town and the dorm, is refused there too unless `MQ_COOP_CAST_FORCE=1`). Relationship tests start from high
  values in the `.coop` file.
  `MQ_MAGIC_LOG=1` logs the incantation events' tests and what coop_magic does.
- The Gateway's destinations (docs/COOP.md, "The Gateway's destinations together"): `MQ_GATE_ANYWHERE=<file>` sets the
  day's Anywhere flag (g1262 and its copies) while the file exists; `MQ_GATE_KEY=<file>` lets the other world's item
  test pass while it exists; the other world itself: poke g1000 = 32 and g1010 = 1 on the host after connecting.
  day1done.sav's students have s411 set (the principal hasn't told them about the Gateway: only the room and Save):
  poke `var 411=0` *before* connecting (a guest's own poke after connecting did not hold). Open the Gateway with
  `MQ_EVENT_FILE` = 25001 only where the student stands at it (upstairs in the dorm, where Connect now leaves both):
  its walk into the Gateway is relative, and run elsewhere (once, on the beach) it walked the student into the beach's
  exit and pushed the other student out too. Harness: `/home/claude/run/live/gwpair.sh` (`PRE_POKE`, `GW_ENV`, each
  copy with its own `MQ_MYST_GOTO` file) and `gwtest.py <user> <partner> <pick> <yes|no|none> <tag>`.
- `--screenshot-at-frame <n> <file>` can be given several times in one run.

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
- Two builds at once in the same build folder break each other (missing object files). Check with `pgrep -x cmake`;
  a `ps | grep -c "[c]make --build"` count includes the checking shell's own command line and never reads 0.
- `git commit -a` in a working tree other agents are editing commits their unfinished work too: commit explicit
  paths (`git add <file>`; for a shared file, your hunk alone via `git apply --cached`).
- Reading a screenshot quickly: a camera pointing at the sky was once taken for a working feature and shipped.
