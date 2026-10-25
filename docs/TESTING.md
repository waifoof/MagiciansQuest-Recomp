# Testing efficiently

The test machine is small (2 cores, 8 GB) and shared by several runs at once. Every test costs minutes, so pick the
cheapest one that answers the question and keep the machine clean. (The harness scripts live outside the repo, in
the session's /home/claude/run; the switches they use are in the game.)

## Cheapest first

1. Static reading: the disassembly and the event script decoder. No CPU.
2. One copy of the game (software renderer, unpaced, `NTR_DRAW_EVERY=10`).
3. Two connected copies (instant connection: about a minute from boot to connected).
4. OpenGL runs (Mesa llvmpipe, 5-10 frames a second): only for what needs the layer split (the PC view).
5. The full regression: once per push to main.

## Switches that save time

- `NTR_DRAW_EVERY=<n>` (software renderer): the renderers draw 2 frames in n; the game runs the same. About twice as
  fast; pictures at most n frames old (0 for a fresh picture).
- `MQ_COOP_AUTO=host|join` (or F9, or Settings > Network > Connect automatically): a co-op visit at once, no gateway,
  no menus. `NTR_LINK_NAME=<word>` gives a set of test copies their own link, so pairs don't meet each other.
- `MQ_WARP_FILE=<path>`: write `e <arrival point>` (outside the café 235, books 243, Harrows 233, school 240, dorm
  221; a building's inside is its door's number + 1) or `<map> <x> <z>` and the player is sent there. Close any open
  talk or menu first: it carries over a warp.
- `MQ_EVENT_FILE`, `MQ_COOP_POKE_FILE`, `MQ_COOP_CAST_FILE`: start one of our events, write a variable, cast an
  incantation, instead of playing up to it. Relationship tests start from high values in the `.coop` file.
- `--screenshot-at-frame <n> <file>` can be given several times in one run.

## Regression tiers

- Quick: the first 20,000 frames (the dorm, the school, the principal's welcome), about 2.5 minutes, both screenshots
  pixel-identical to the baseline. For changes that only act in co-op or the PC view.
- Full: 80,000 frames, about 11 minutes, eight screenshots pixel-identical to the baseline. Once per push to main,
  batching verified features; built from the exact commit in a clean worktree, so nobody's unfinished edits are in
  the tested binary, and run through a renamed link to the binary, so kill patterns aimed at other runs can't stop it.

## Don'ts (each cost hours once)

- A debugger breakpoint left on code that runs every frame (Python or conditional) slows the game about ten times.
  Use gdb only to read or write once and detach; put research logic in an env-gated branch of a hook instead.
- Killing every game copy by name stops other people's runs (regressions died this way). Stop your own copies by a
  pattern unique to your save path, in a separate command from any launch.
- A wait loop that `pgrep -f`s a pattern that appears in its own command line never ends.
- Killed runs leave their virtual displays (`Xvfb`, about 67 MB each) behind: clean the ones whose parent is gone and
  that are over an hour old.
- More than two or three game-running jobs at once on two cores only slows all of them. Agents that only read and
  write code are cheap.
- Reading a screenshot quickly: a camera pointing at the sky was once taken for a working feature and shipped.
