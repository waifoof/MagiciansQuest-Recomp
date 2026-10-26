# PC view: one screen, rendered as it should be (plan)

A new layout alongside the existing ones (stacked, side by side, one screen with its joined/flat/solo arrangements,
main + side, two windows). Not an arrangement of the two DS framebuffers: one picture of the world from one camera,
with the DS's 2D shown as overlays and panels.

## References (summary)

- No shipped PC port of a dual-screen 3D DS game renders it as one camera's view; emulators (melonDS DS hybrid
  layouts, DraStic, Citra/Azahar custom layouts) only arrange framebuffers.
- Official remasters converge on: the world in one big view; the second screen's content as on-demand overlays or
  small panels (Castlevania Dominus Collection status/map windows, FF IV 3D remake's toggle map overlay); menus as
  centred modals opened with a button (Ace Attorney trilogy's court record on R, Wind Waker HD's TV menus); touch as a
  pointer with feedback plus button alternatives for frequent touches (Etrian Odyssey Origins).
- Pitfalls: important content on the other screen (in this game: the night sky, whose stars matter), menus that use
  both screens (info on top, input below), gesture touches (chores, music, minigames), input that must map back to
  exact DS coordinates, layout flicker at scene changes, and in co-op every touch must go through the input record.
- 2D scaled by whole numbers; HUD anchored to edges with a size setting; widened 3D needs the game's 4:3 culling
  handled.

## Design

| scene | PC view |
| --- | --- |
| field (outdoors) | one wide 3D view from the bottom screen's camera, extended sideways to the window and upward into the sky (about 70% ground, 30% sky by default; "look up" eases to the full sky); bottom-screen HUD clusters in the lower corners, clickable; the top screen's date/time/place in a corner widget |
| indoors (3D on the bottom only) | the bottom 3D fills the window; top-screen text slides in as a banner when it changes |
| dialogue | text box as a lower-third panel at an integer scale; touch choices next to it, clickable |
| bag, spells, letters, map, personal info | a centred modal over the dimmed world: the two DS screens side by side, info pane left, touch pane right (only the right one takes clicks) |
| title, character creation, keyboards | the same modal; the physical keyboard types into the DS keyboard |
| minigames, chores | per scene: the single view when the action is 3D, else the side-by-side modal |

Input: the mouse is the stylus everywhere (hit order: modal pane, HUD clusters, the 3D rectangle; clicks in the 3D
outside the DS bottom rectangle are ignored, never clamped); a context cursor; controller buttons first, a snapping
right-stick cursor only in modals.

## Build order

1. Single-camera field render (in testing): both passes from the bottom screen's view object, the top pass through a
   projection window lifted one screen (see below); later one projection per frame at 60 fps. On with the PC view
   layout (`MQ_PC_VIEW=1`: in every layout). Never while a co-op link is set up or the wireless is in a session: its
   hooks write the game's memory, and every console of a session must run the same game; the layout then shows the
   bottom screen alone (as indoors, see 3). The hooks act only while the field's own camera is live outdoors (see
   "When the PC view acts" below).
2. Layer split in the renderer (written, not yet run): at the VBlank, engine A's screen is composited twice more
   (`FfccLayerSplit`: 1 the world, the layers up to the 3D's place in the priority order; 2 the 2D after it, with
   alpha), each through the final pass, into per-screen arrays (`GlLayers` in `runtime/src/gl_between.cpp`,
   `Frame::gl_world`/`gl_overlay`). OpenGL and compute only; the software renderer shows whole screens.
3. Presenter (running, OpenGL): the `PcView` layout (`pc-view`). Joined (only while the game's camera hooks act,
   `cfie::pc_view_acting`, held 8 frames): the bottom screen's world whole, under the lower 43% of the top screen's
   (about 70% ground, 30% sky; `video::pc_view_sky`, `MQ_PCV_SKY`; the top pass still renders the whole lifted screen,
   the presenter crops it with `ScreenView::crop_top`), filling the window (the renderer's extra width follows the
   ground's height, so it is about half of the 50/50 split's (116 rather than 212 at 16:9), rounded up to a step of 4,
   and the middle that fits is shown: no side bars), the bottom screen's 2D over its
   part, the top screen's 2D as an unframed corner panel shown only while it has something drawn (its alpha measured
   by the probe at 128 x 96, `top_overlay_peak`; hidden 2 s after, and while the top screen is blank), and over the
   top part during fades, the wide sides darkened by the fade's weight (BLDALPHA EVA, `Runtime::blend_alpha`) so the
   whole width fades. A screen whose parts are old (no 3D on it now) shows only its DS pixels, never its wide sides
   left from the last 3D frame. Elsewhere with 3D on both screens (rooms, halls), and with 3D on the bottom only: the
   bottom screen alone fills the window (One screen's solo, zoomed); its corner picture sits in the margin beside the
   touch screen's DS rectangle (the classroom's talk button is in that corner) and clicks never stop at the top
   screen in the PC view, they reach what is under it. No 3D, or 3D on the top screen only (the Start menu's Wizard
   ID portrait over its touch menu): the menu modal (see "HUD, text box, menus" below). Touch only through the bottom
   screen's part (`screen_at`). World extension (world_ext.cpp) is always on in the PC view. The layer arrays sample
   with CLAMP_TO_EDGE (the seam at the join and the bright outer rows were GL_REPEAT wrapping).
4. Scene detection (running): menus from the game's 3D window slide (hook-read), the text box from its draw hook plus
   the probe, the HUD with the touch screen's 3D shown; see "HUD, text box, menus" below. The joined/solo/flat choice
   is still the 3D-presence heuristic held over frames.
5. Settings UI rows; reviewers (agents) go over each scene for inconsistencies.

## The field's camera (research)

- One projection (main 0x020093D0: fovy as a half-angle, 15 degrees; D = 96/tan(fovy) = 358 px), built once and shared
  by both screens; its window can be shifted in pixels (shorts at 0x020AD7D8 vertical, 0x020AD7DA horizontal; the
  field leaves them 0).
- The 3D is drawn one screen per frame (parity bit 0 of 0x020ACA55: 0 top, 1 bottom). Each pass builds its camera from
  that screen's view object (0x020AD7B8[parity], look-at main 0x0200B7BC). In the town the two view objects
  (ov001 class 0x020FD528) share the eye's x and z and the yaw; the top one is pitched 36.6-39.3 degrees higher
  (`/map/field/CameraParam.dat`, 28-byte records: eye height and pitch per screen). The DS never shows the 7-9 degree
  wedge between the screens.
- Per-screen state besides the camera: fog (0x020AD7C4 / 0x020AD7E0 per parity; the town sets 0x100 top, 0xD0
  bottom), engine A's 2D layers and scroll, and a few things drawn in the top pass only (ov001 0x020D7C24 and
  0x020D7DB8, probably the sun or moon and sky).
- The PC view (step 1): hooks `mq_view_camera` (0x0200B7BC: the top view object -> the bottom one), `mq_view_of`
  (0x020094C8) and `mq_view_pass` (0x02009204: the top pass's window lifted by `MQ_PC_VIEW_LIFT`, 192 px by default).
  The bottom pass and the touch screen are unchanged. Switched by the frontend (`cfie::pc_view`, see build order 1).
- When the PC view acts (`mq_pcv_field`, decided at each pass's beginning, 0x02009204, and held through the pass):
  3D on both screens (0x020ACA54), both view objects of the field's class (ov001 0x020FD528; the dorm halls use
  another, 0x020FD5E8, and the intro and title cameras 0x020A324C), and the top one pitched at least 20 degrees above
  the bottom one (town: 34-42; `MQ_PCV_MIN_UP`). The Start menu (Wizard ID, magic, telegrams, info), shops and the
  classroom have 3D on one screen only, so the hooks stay off there. The bag keeps the field camera, so it stays on.
- The bag bug (fixed): opening the bag, the game slides the bottom camera's window sideways (0x020AD7DA 0 -> 45 in
  steps) and writes 0x020AD7D8 = 0 as it does. The old `mq_view_pass` worked out the game's own value as "memory minus
  our lift", so a write made while the lift was in memory turned into -192 for every bottom pass (the bottom camera
  looking at the ground below; it stayed after the bag closed). Now the top pass saves the game's value, writes it +
  the lift and sets the projection up, and the frame's end (`mq_view_pass_end`, from mq_frame_end, before the 3D swap)
  puts the game's value back (unless the game wrote its own meanwhile) and sets the projection up again, so the
  game's logic and stylus picking never see the lifted window. `mq_view_of` now redirects only during the pass (the
  game's own updates of its top camera between passes reach the top view object). With the PC view off, every hook
  returns before touching memory. Research switch: `MQ_PCV_LOG` (each pass's view object, class, eye and target,
  window shorts, and the view_of callers).
- The night sky (found and fixed): it is 3D, not a 2D layer (engine A shows only BG0 = 3D, BG3 in front and sprites;
  DISPCNT is the same on both parities). The game keeps its camera in the projection (NNS: projection x camera), and
  draws the sky through the inverse camera matrix (main 0x020094B4 -> 0x02001CA0, cached at 0x027E0160, valid bit 8
  of 0x027E015C), so it sits in camera space, at fixed screen places: stars, constellation lines, moon and planet
  (ov001 0x020DB2C4 -> 0x020D9B70: camera-space (x/128, y/128, -127)), the weather (ov001 0x020D84B4 ->
  0x020D7C24 / 0x020D7D58), and the sky's colour bands (main 0x0202A4BC / 0x0204DF5C: models flagged to draw in the
  top pass only; 0x0204FC64). All are drawn in the top pass only, sized to the DS top screen. Through the lifted
  window they fell one screen below the view, so the top half was the black rear plane (with the top camera kept and
  only the lift, `MQ_PCV_NOCAM=1`, the same; the view-object pitch, fog and 2D were not involved). Fix
  (`mq_view_inverse`, instead at 0x020094B4): in the lifted top pass, those callers get the inverse with a camera-space
  shear y += k * depth (k = lift / 96 * tan(half fovy); row z of the 4x3 inverse -= k * row y), which puts every
  point back where the DS top screen had it through the lifted window (flat sprites keep their shape; depth is
  unchanged, so buildings still cover the sky); the valid bit is cleared so other callers get the true inverse.
  Research switches: `MQ_PCV_SKY_OFF`, `MQ_PCV_NOCAM`; `MQ_GXLOG_PARITY=1` with `MQ_GXLOG` splits the GX caller
  log by pass (`p0` top, `p1` bottom).
  Risks: geometry the game streams only for its own views (pop-in
  above), fog stepping at the join, a one-frame tear at the join while the camera moves (each half is 30 fps).

## Review fixes (2026-10-24) and what is left

Fixed and checked with screenshots (software for the camera logic, OpenGL for the picture): the bag no longer breaks
the bottom camera (it slides the view as on the DS, and the camera follows the player after it closes); the Start
menu, shops and the classroom leave the hooks off; the join has no seam; no side bars; rooms and halls show the bottom
screen alone instead of a black upper half; the corner panel hides when the top screen's 2D is empty; a screen with
old parts shows only its DS pixels; the classroom's corner picture sits beside the touch area and never takes clicks.

Left: see the end of the next section.

## HUD, text box, menus, fades, look up (2026-10-24, second pass)

All in the presenter (`game/src/video.cpp`, `main.cpp` `update_pc_parts`), from the renderer's layer split (OpenGL
only: with the software renderer none of this acts and the screens are whole). A `ScreenView` can now show a part of
a screen (`src`, DS pixels) on a place of its own, drawn `crisp` (nearest, at a whole-number scale: the window's
height / 360, 2 at 720p, 3 at 1080p); the screen's whole 2D leaves those parts out (`holes_`, a discard in the screens'
shader, widened a pixel so filtering leaves no edge behind), and `screen_at` maps a click on a part to where the part
is on the DS and ignores clicks where it was. Script testing: `click x,y [frames]` in an input script or the live
control file is a click at a window position (pixels) through the layout, as the mouse's (`MQ_LOG_ONE` logs where
each lands).

- HUD (joined and the touch screen alone): the touch screen's top corners, DS (0, 0, 40, 40) (the bag) and
  (216, 0, 40, 40) (the hand, or the tool the student holds), are drawn in the window's lower corners (a margin of
  1.5% of the height). Not while a menu or a fade is up (BLDCNT 0x3748 with a weight, or the probe seeing the touch
  screen's 2D opaque over nearly all of it: the curtain while a scene is set up, which would otherwise show the
  world through the HUD's holes). Clicking the bag where it is drawn opens it (checked: window
  (47, 661) -> DS (18, 16)).
- Text box: the game's draw of its text box (`mq_text_box_draw`, now counted: `cfie::text_box_draws`) arms it; the
  game draws the box only while it writes the text, so it stays while the probe of the touch screen's 2D (64 x 48,
  `bottom_overlay_box`) sees its bottom 16 rows drawn across most of the width. The touch screen's rows from the box's
  top (the probe's `bottom_overlay_top`, the lowest row from which every row down has something drawn: the box, its
  name tag and the choices above it; at most row 112, at least 44, only growing while the box is up) to its bottom
  edge are drawn at the bottom middle of the window, crisp, at the whole-number scale; the choices inside are clicked
  there. In the classroom (touch screen alone, zoomed 3.75x) the box was 960 x 300 px; now 512 x 160 at 720p.
- Menus: a modal, the two screens side by side at the largest whole-number scale that fits 96% of the width and 92% of
  the height (2x at 1280 x 720), in the middle, framed, over the picture from before it opened, darkened to 40% (kept
  every frame the world is shown settled, in three layers written in turn every 10 pictures: the modal shows one 10-20
  pictures old, never a frame the game drew as the menu opened). Clicks reach only the touch screen's pane. Open while
  the game slides its 3D window sideways (the short at 0x020AD7DA, read in `mq_frame_end`, `cfie::pc_view_menu`): the
  bag (0 -> 45 in steps) and the Start menu's pages (-74, its Wizard ID portrait); also whenever the arrangement is
  flat (no 3D: title, character creation, other 2D menus). Research: the task slots at 0x020ACE00-0x020AD000 carry
  names (`item_slot` while the bag is up, `pic_stat_c`/`map_change` in the Start menu), but slots keep the names of
  finished tasks; `MQ_PCV_MENU_LOG` logs the window's moves.
- Fades: the black sides were not the camera. The game fades with a black background in front of the 3D blended in
  (BLDCNT 0x3748), and the compositor's "a black background in front at the edge, a curtain: the sides black too"
  rule blacked the world's wide sides for it, so only the 4:3 middle faded in. Now the PC view's world (uSplit 1)
  ignores a black edge that is a blending first target (`third_party/CMakeLists.txt`), and the presenter darkens the
  wide sides by the fade's weight with the touch screen alone as well as joined (`fade_dark_`). The parts were fresh
  and the scene wide (FFCC_LOG_WIDE, `MQ_LOG_ONE`'s parts log) all through the classroom's fades: no ortho block, no
  projection of the game's own.
- Testing: the probe measures only presented pictures (every 8th); a test run with `--present-every 30` probes about
  every 240 frames, too seldom for the text box and the curtain: use `--present-every 1` for those (slower).
  `MQ_LOG_ONE` logs the parts' age, the probe's results and the text box draws on change; `MQ_LOG_PROBE` every probe.
- Look up (`Action::LookUp`, key E, bindable; testing `MQ_PCV_LOOK=<from>-<to>` frames): while held, the joined
  picture eases (a quarter second) up the two screens' picture, the same height shown, to the top screen whole above
  the upper part of the touch screen (`crop_bottom`; touches follow), and back when let go. The HUD and text box stay
  anchored to the window.
- The corner panel slides in and out already (the arrangement's move when it shows or hides: off the window's right
  edge and back), on new 2D on the top screen.

Screenshots (1280 x 720, OpenGL llvmpipe, the regression's input, software until a frame then OpenGL), in
/home/claude/run/live/: pe2/t20000.png (classroom text box at 2x; before: pa/x20000.png at 3.75x), pe2/f20140-20250.png
(the classroom's curtain and fade in, full width; before: pb/x20210.png, 4:3 with black sides), pf/a_field.png (HUD in
the lower corners), pf/b_bag.png (the bag's modal, opened by clicking the moved bag), pf/d_start.png (the Start menu's
modal), pf/f_startclosed.png (closed by clicking its X in the touch pane), pc/h_look.png (looking up: the sky and the
constellation; pc/h_before_look.png before).

Left (updated 2026-10-26, see "The remaining screens" at the end):
- The wide sides: the town's cell windows are widened (see "The wide sides' geometry" below); other outdoor areas and
  the sky bands' far ends are not looked at in pictures.
- The HUD corners are fixed rectangles of the DS screen (the town's bag and hand); a HUD the game puts elsewhere
  (spell gems, the classroom's talk button) stays in place, at the zoomed screen's scale.
- In a test, a tap on the 3D beside the open bag (DS 246, 182) closed the bag and set the student casting in the PC
  view, but only closed it in the DS reference run (same inputs, a few frames apart): not followed up (timing, or
  the widened projection in the game's stylus picking).
- Choices the game shows with no text box under them (the post office's "Other player / Classmate / Exit") stay at
  the zoomed screen's scale: the band starts from a box at the bottom (or top) edge.

## The wide sides' geometry: the town's cell windows (2026-10-25)

The town (ov002) is a grid of 22 x 26 cells of 4.0 units (x across, z away from the camera; cell = position >> 14 in
20.12). It does not cull per object against the view: it builds render objects only for windows of cells around the
camera, again whenever the camera enters another cell (the rebuild, ov002 0x02100278 = vtable +144 of its field class
0x021068B8, called from its update 0x02101538):

| window | what | columns | rows | list |
| --- | --- | --- | --- | --- |
| 1 | map cells (buildings, walls, ground pieces; 0x020FFE90) and their items (0x02101C54) | 4 from (camera x - 6.0) / 4 | the camera's and the one before | render objects at town +0x172E4 (232 bytes, end word +0x19554), animation links at +0x19558 (28 bytes, end +0x19980): 38 each |
| 2 | the same, each row only pieces of its level or more (items in the first 3) | 6, one further left | the next 5 | (same) |
| 3 | objects (trees, lamps, planters; 0x02100028) | 6 from (camera x - 10.0) / 4; the outermost columns only big ones (an edge flag) | 3 | town +0x19984 (224 bytes, 288, end word +0x29584, its own capacity check) |
| 4 | the same | 8, one further left | the next 4 | (same) |

Those windows fit the DS's 4:3 view; at 16:9 the picture showed the renderer's fill (the hole fill and world
extension's ground) where the next columns' pieces belong. The leads in the old notes were not it: main 0x0202A4CD is
the sky bands' draw (0x0202A4BC, its stack entries in the GX log are stale words), ov002 0x021034D9 / 0x0210356D are
collision lookups (point in a cell's rectangle), and the hardware box test (0x0200CC0C) only decides among pieces
already built (it is why forcing it changed nothing).

`game/src/town_cells.cpp` (hooks `mq_town_cells` instead at 0x02100278, `mq_town_objects` at 0x02100028,
`mq_town_update` at 0x02101538, `mq_town_draw` at 0x02101724): with the view wider than 4:3 the rebuild runs the
game's own and then the same functions for the columns beside each window (per side: ceil(half width in columns x
(aspect / (4:3) - 1)): 2, 3, 3, 4 at the PC view's 16:9), nearest rows first. Objects and items go into the game's own
lists (it updates, draws and releases them), and the objects' edge flag moves to the new outermost columns. The map
cells' lists are always full with the game's own windows (38 of 38 in the town), so the extra map cells are render
objects of ours, constructed as the town's constructor builds its own (ctor ov002 0x0210316C, links main 0x0200D5A4),
in guest memory the DS does not have (`ntr/hook_ram.h`: 64 KiB at 0x0B000000, mapped by the runtime like main RAM):
released at the next rebuild (0x0200CDE8 / 0x0200D770), stepped after the town's update (0x0200CD04, under the same
flag/pass test) and drawn after the town's draw (vtable +32) under its conditions (bottom pass of two 3D screens:
pieces flagged at +224 only when the box test passes; otherwise a piece's record flags and the player's nearness), and
never touched again once the town they were built for is gone (its list end and first model must match). Gate:
`cfie::wide_cells` = "Build the world beyond the view" or the PC view, and no co-op link or wireless session; and the
aspect wider than 4:3. At 4:3 the hooks only call the game's functions (the quick regression passes). Research:
`MQ_CELLS_LOG` (each rebuild's windows and counts; a draw summary every 240 frames), `MQ_CELLS_EXTRA=<n>` (n columns
per side for every window; 0: the game's windows, the "before" for comparisons).

Checked (OpenGL, PC view, 16:9, the regression's input, `MQ_CELLS_EXTRA=0` against the default): 16-28 map cells of
ours per rebuild in the town walk, objects 22-29 (game 14-22); new real pieces where the fill was, e.g. the planter
wall and rock planter at the right at frame 31400, the cobbled path's far end at the left at 30300; nothing else in the
picture changes. Shots: /home/claude/run/regs/walkA (before) and walkB (after), w31000-w34200; cullA (before) and
cellsF (after), t30000/t30300.

Not done: the other outdoor areas (other overlays, e.g. ov003; not looked at); the sky bands' far ends (drawn sized to the DS top screen). NPCs are not culled at the
4:3 edge (drawn whole beside it in the walk shots).

## The remaining screens (2026-10-26)

Gone through in OpenGL pictures (1280 x 720, `--set-at-frame` / the live control's new `set graphics.renderer=opengl`
near the scene, from day1done.sav and a blank save; shots in
/tmp/claude-0/-home-claude/27590c79-a0d0-5540-a75e-7e180d41e293/scratchpad/pcv2/):

| scene | what the game draws | PC view | checked |
| --- | --- | --- | --- |
| shops: Daisy's (berries), Vivian's Boutique (clothes: "That?", Try on, Buy), Harrows (the clerk's talk; its furniture stands on the floor, the buy flow from an item not walked) | the room in 3D on the touch screen, the talk box and its choices / lists in 2D over it; Try on is a 3D scene in the fitting corner | the touch screen alone (the room fills the window) with the text band; no catalogue screen with 3D was found in these shops (Daisy's and Vivian's goods are picked in the room and bought through choices), so no modal is needed there | dz3_choices.png, dz3_berries.png, g_tryon1-3.png, g_h1-3.png |
| Start menu: info (personal info), magic, incantations, town map, telegrams (letters), friends | 3D portrait on the top screen, 2D pages on the touch screen | the modal, all pages (the window slide -74 holds) | tabs.png (g_tab815..1040, g_start) |
| keyboards: the name at enrolment, a telegram's text (ov035 "input_str") | 2D on both screens | the modal; the PC's keyboard types into it (below) | kb_after2.png ("Mo 2b" typed) |
| the telegram's "WHO" picker (ov009, a 3 x 5 grid of classmates) | 3D clerk on the top, 2D grid below | the modal (flat) | p7.png (software) |
| the school bell (Organ Man, ov013 /chime/: staff, OK / record / play, a piano) | 3D room on the top screen only, the composer in 2D below | the modal (3D on the top screen only) | g_bell2.png |
| fishing, insects, the hotpot cook-off's mushrooms, detention's cleaning | the field or a room in 3D | the joined view / the touch screen alone (3D action); not reached in this pass | |

**The text band, as high as its choices.** The probe of the touch screen's 2D counted every drawn pixel of a row, so
its band was clamped at row 44 (below the HUD's corners) and a tall list (Daisy's menu at row 28, the berry list)
lost its top: the list's frame was left at the zoomed screen's scale above the band (before: g_d3.png). In the HUD's
rows (DS 0-43) only what is between the corners (DS 40-215) now counts, the band may start at row 0, and the HUD's
corners are left in place while the band covers their rows (after: dz3_berries.png).

**A box at the top of the touch screen.** The probe also measures from the top edge down (its top 16 rows drawn
across most of the width, and the lowest row everything above reaches); a box there with none at the bottom is drawn
at the top middle of the window, crisp, its rows down to that row (`set_pc_text_top`). Not seen in a picture yet: no
scene putting the game's talk box at the top was reached (ambient balloons such as Vivian's "Dun-di-dun..." are
narrower and stay in place).

**The PC's keyboard into the game's keyboard.** `ov035 0x0211B9B4` (the keyboard's touch handling, called from its
task's loop every frame) is counted by `mq_keyboard_frame` (nothing read or written). While it counts and the layout
is the PC view (`pc_keyboard`), SDL text input is on; a typed character is tapped (4 frames down, 4 up) on its key
after the page tab it needs (ABCD at DS 46,45; SYMBOLS 101,45; the ABCD page comes back in upper case) and the case
(LOWER 34,146; UPPER 76,146), both remembered until the player's own touch; keys at DS (24 + 20.95 col, 66 + 16 row)
in an 11 x 5 grid (ABCD: A-F & % ~ - / G-L @ _ " # / M-R : ; | , / S-X ! ? / \ / Y Z [ ] < > ( ) { }; space and the
cursor keys in column 10; SYMBOLS: the digits, . + = * ^ '). Backspace taps DELETE (202,146), Enter OK (194,180),
Shift+Enter the new line key (234,146), the arrows the cursor keys. The DS buttons' keyboard bindings are off
meanwhile (a gamepad's still work); the function keys still act. Testing: `type <text>` and `kbkey
backspace|enter|left|right` in the live control file. Verified: enrolment's name typed as "Mo 2b" (lower case after
a digit included).

**The faint vertical line in the sky.** The one picture with it (pc/h_before_look.png, x 617, y 17-139) shows a thin
light outline with a short horizontal end, drawn behind the buildings (so in the 3D, not the 2D panels); its size and
colour match the constellation lines of the night sky (pc/h_look.png: HAPPY's strokes, 125 px tall). Fresh runs of
the town at night (regs/pz_sky, t30000-t30550, looking up from 30200) show no such line, and a scan for one-pixel
vertical lines in their sky parts found only the constellation's own strokes. Not reproduced; if it comes back, check
whether it is a constellation stroke drawn through the unsheared inverse (it would sit about one screen lower than
the DS has it).

**Other outdoor areas.** The areas (ov003; town exits 0x9C, 0xAE, 0xA6 to maps 63, 64, 66, the beach) do not build
cell windows around the camera as the town does: their rebuild (vtable +144, 0x02101F40) walks the area's whole object
list (208-byte entries) and its update (0x02100324) sets up a view test from the camera each frame (0x020094B4,
0x02109084, 0x02109124). So town_cells.cpp's approach does not apply; whether that view test uses the widened
projection (cfie::aspect) was not checked in a picture (the `e <n>` warps tried for those exits are not arrival
points; walking there is the way).

Live control (testing, `--input-live`): `set key=value` (as `--set-at-frame`, e.g. `set graphics.renderer=opengl`
right before the scene to look at), `type`, `kbkey`.
