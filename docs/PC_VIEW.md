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
   projection window lifted one screen (see below); both parts of the picture from the same game moment, at 60 fps,
   through the interpolation of the two screens (see "One moment" at the end). On with the PC view layout
   (`MQ_PC_VIEW=1`: in every layout). Never while a co-op link is set up or the wireless is in a session: its
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
  above), fog stepping at the join, a one-frame tear at the join while the camera moves (each half is 30 fps; fixed,
  see "One moment" at the end).

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
- The wide sides: the town's cell windows are widened (see "The wide sides' geometry" below); the other outdoor areas
  draw their whole object list (filled), and the sky bands reach the edges (see "One picture" at the end).
- The join between the two passes: one scene since 2026-10-27 (see "One picture" at the end), and one game moment
  (no tear while the camera moves, see "One moment" at the end).
- The HUD corners are fixed rectangles of the DS screen (the town's bag and hand). Looked at on 2026-10-27 (see
  "The remaining screens, part 2"): the classroom's HUD is the same corners (the bag; the hand only with someone to
  talk to), and the tournament's word tiles are on a 2D touch screen under 3D on the top screen only (the modal). A
  corner is copied only while the game draws its HUD there (since 2026-10-27, see "Fishing" at the end).
- In a test, a tap on the 3D beside the open bag (DS 246, 182) closed the bag and set the student casting in the PC
  view, but only closed it in the DS reference run (same inputs, a few frames apart): not followed up (timing, or
  the widened projection in the game's stylus picking).
- Choices the game shows with no text box under them (the post office's "Other player / Classmate / Exit"): done
  2026-10-27, see "The remaining screens, part 2".

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

Done since (see "One picture" at the end): the other outdoor areas (ov003) looked at, the sky bands' far ends.
NPCs are not culled at the 4:3 edge (drawn whole beside it in the walk shots).

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
| the school bell (Organ Man, ov013 /chime/: staff, OK / record / play, a piano) | 3D room on the top screen only, the composer in 2D below | the modal (3D on the top screen only); the PC's keys play the piano (below) | g_bell2.png, bell_after.png |
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

**The bell's piano from the PC's keys.** `ov013 0x0210AD08` (the composer's step, each frame) is counted by
`mq_bell_frame`; while it counts in the PC view (`pc_piano`), the keys play its piano by their places on the PC's
keyboard (scancodes): the home row A S D F G H J K L the white keys Do Re Mi Fa So La Ti Do Re (DS y 175), W E / T Y U /
O the black ones (y 151), Enter OK, Space play, Backspace the X; the DS buttons' keyboard bindings are off meanwhile.
Verified: A D G K W tapped Do, Mi, So, Do and Do# (the staff showed #C under the stylus on the Do# key).

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
right before the scene to look at), `type`, `kbkey` (with the bell's piano up: a key's name, `kbkey A`).

## One picture: the join, the fog, the sky's far ends (2026-10-27)

The town in the PC view still read as two bands: the far scenery (what the DS top screen shows) ended in a straight
line about 0.3 of the way down (y 216 of 720 at 1280 x 720: the join of the two passes), the near ground below it.
Not the camera, the window or the cell windows: both passes already use the bottom screen's view object, one
projection and the same render objects. **The town's draw picks its pieces per screen** (ov002 0x02101724, the
field's vtable +116):

| list | bottom pass (3D on both screens) | top pass |
| --- | --- | --- |
| map pieces (town +0x172E4, 232 bytes) | every one; those flagged at +224 when the hardware box test (0x0200CC0C) passes | only when +0x15242 is set (the town sets it), and then a piece whose record (`*(town+0x96C) + id*12`, id at +222) has bit 3 of byte 7 (building fronts, walls, the far scenery), or bit 7 with the student low (y < 2.0) and within 5.0 in z |
| objects (town +0x19984, 224 bytes) | every one except kind 1 not shown (+96 = 0) | kind != 1 and (+96 or bit 7 of byte 2 of its record `*(town+0x970) + id*12 - 12`) |
| shadows (town +0x29DB0, count +0x31BB0; 0x0200CCDC) | all | none |

So through one camera the near pieces (planter walls, the riverside ledge, the ground's raised edges, the pampas
grass, the shadows) were drawn only below the join and the DS top camera's pieces only above it. Fix
(`mq_town_draw`, town_cells.cpp): in the PC view's top pass (`cfie::pc_view_acting`, 3D on both screens, parity 0)
the game's draw runs as it is, then the pieces it skipped there and the bottom pass draws are drawn, under the bottom
pass's rules (box test for the flagged ones); our wide map cells use the bottom pass's rules in the top pass too. The
top pass's frustum clips or box-tests away what it does not see (49 extra draws per top pass in the town walk).
Research: `MQ_PCV_TOP_GAME=1` (the game's top pass, the "before"); `MQ_CELLS_LOG`'s draw line counts them (`top+`).

**The fog.** The game sets the fog per pass at its frame's end (0x020092A8 -> G3X_SetFog main 0x02087E10: offset
0x020AD7C4[parity], a word each; shift 0x020AD7E0[parity], a byte each); the town's top screen starts it farther
(0x100, the bottom 0xD0). The PC view's bottom pass gets the top pass's (written in `mq_view_pass_end` before the
game reads it, put back at the next pass's beginning unless the game wrote its own): with the bottom's in both, the
far shops in the upper part faded into the fog. The bottom part's pixels barely change (its depth range ends before
the fog). Research: `MQ_PCV_GAME_FOG=1`.

Measured (mean of |row 216 - row 215| over the width, the other rows' median 7-11): 53.6 -> 25.6 at frame 30000,
86.6 -> 45.7 at 30300 (walking), 61.8 -> 23.6 standing at 30200; the remaining step is real edges that happen to lie
there (the shops' bases: a picture looking up, where the join is elsewhere, shows no step) and, while the camera
moves, the tear below.

**The one-frame tear** (each pass on its own frame): fixed, see "One moment" below.

**The other outdoor areas (ov003: the beach 64, the forest 66, Lovers Point 49).** Reached by `MQ_WARP_FILE` (`64 5
14`, `66 5 14`, `49 6.2 3`) from day1done.sav after the dorm's morning talks (a warp started during Reginald's "Hey
..., I've been waiting for you" carried the talk along and it never closed). Their wide sides are filled: the areas
draw their whole object list (the rebuild 0x02101F40 walks it; their update 0x02100324 is no view test: it places a
backdrop object at the camera, area +0x45754, with the inverse camera's rotation). Their draw (0x02100608) also
picks per pass (its second list, area +0x198E0, under its record flags in the top pass), but nothing cut at the join
was seen in the three areas' pictures.

**The sky's far ends.** The colour bands (0x0204FC64's quads, the band models of 0x0202A4BC / 0x0204DF5C) and the
areas' backdrop are sized to the DS screen; beyond them the renderer's fill showed: stretched colour blocks in the
upper corners (the beach, the town at night looking up) and, from the bottom pass (which draws the bands' lower end
where the DS bottom screen shows sky), a block beside the beach's horizon. `mq_view_inverse` (0x020094B4) now also
widens these callers' inverse in camera space (row x times aspect / (4:3), x1.15) in both PC view passes; the stars,
the moon and the weather keep their shape (they are not scaled). Research: `MQ_PCV_BANDS=<mask>` (1: 0x0204FC64,
2: 0x0202A4BC, 4: 0x0204DF5C, 8: the areas' backdrop; 0: none).

Pictures (1280 x 720, OpenGL llvmpipe) and labelled before/after composites in /home/claude/run/pcv3/:
fix1_seam_t30000.png, fix1_seam_t30300.png (the regression's input; before = regs/cellsF), fix1_seam_still30200.png
(standing; before = the same build with `MQ_PCV_TOP_GAME=1 MQ_PCV_GAME_FOG=1`), fix3_bands_beach.png,
fix3_bands_town_night.png (looking up); the areas: areas/beach_bands3.png, lovers_after.png, forest_after.png.
The quick regression passes with each change (the game at 4:3 and outside the PC view is unchanged). The town walk
(the regression's input, 31000-34200, /home/claude/run/pcv3/walk/) shows one scene in every shot. Seen there and
older than these changes (regs/walkB/w32600.png has it too): a one-pixel orange line from the window's left edge to a
classmate's hair, standing beside the 4:3 view (w32600, y 380). It was the world extension carrying the top of the
hair on as ground: fixed 2026-10-27, see "Fishing" at the end.

## One moment: both parts of the picture from the same frame (2026-10-27)

**The cause.** The game draws its 3D one screen per frame (the top pass on one frame, the bottom pass on the next)
and runs its logic every frame: the camera moves between the two passes (the view objects' eye, `MQ_PCV_LOG`,
walking right: +0.066 units each frame, both view objects every frame). Every picture of the PC view was one pass's
fresh scene and the other's from the frame before, so while the camera moved the two parts were one frame of motion
apart, the part drawn last ahead, the other behind, in turn every frame: the join's sideways offset alternated
between about +4 and -3 px at 1280 x 720 (frame-to-frame change 7 px), and anything crossing the join (the near wall
at the left of the town walk, the shop corners) broke there. The interpolated setting (60 fps, the default) tore the
same: each screen showed its blend, and the other screen's part was that screen's blend of the frame before.

**The options.** (a) Showing only pairs (one top pass with the following bottom pass) at 30 fps: the camera differs
between the two frames of any pair, so a pair is still a frame apart; holding the camera for the pair would align
the still world but not anything moving (the student, classmates), and halves the rate. (b) Both passes every frame:
the game's draw code changes its own state (lists, animation, the top pass's sky and weather), and its hardware
flush waits for the VBlank, so a second pass would change the game or cost a frame. (c) Taken: the interpolation of
3D on both screens that already exists (`gx::Streams::dual`): a screen's new scene is shown blended with its scene of
two frames before (the frame between), while the other screen's real scene is of that same frame. The DS layouts
already pair these; the PC view's world layers did not: the other screen's layer was its own blend of the frame
before. Now:

- With the PC view the interpolation is always on (`main.cpp`, `dual_60`; research `MQ_PCV_NO_DUAL=1`: as the
  setting has it). Each picture shows the game one frame late (as the interpolated DS layouts do), at 60 fps.
- At the VBlank where the renderer draws a screen's real scene again (`render_dual_real`, for the frame after), that
  real scene's world is also split into a layer of its own (`GlLayers::split_real`, through `GlBetween::render`'s
  `with_3d`); at the next VBlank, when the other screen shows its blend of that frame, it goes in the world array in
  place of the blend that screen showed (`use_real`, in `layer_split_hook`; only when it was made at the VBlank
  before). Research: `MQ_PCV_SHOWN=1` (as shown), `MQ_PCV_DUAL_LOG=1` (each split).
- The blend missed the camera now and then (2 of 120 walking frames; the whole top part a frame ahead): this game
  keeps its camera in the projection (a projection load and the camera multiplied in, in projection mode), and sets it
  up again before some objects; when such a set-up came or went between the two scenes, no chunk lined it up, and
  everything after it was drawn from the new camera. `gx::blend` now blends projection-mode matrix commands no
  lined-up chunk covered with the most alike command of the same kind in the other scene (unless it jumped);
  `tests/test_gx.cpp` `test_projection_camera_in_new_chunk`. Research: `MQ_GX_BLEND_LOG=1` (each blend: lined-up
  entries, projection commands matched, the first matrix loads), `MQ_GX_CHUNK_LOG=1` (the chunks not lined up),
  `--gx-probe` (with the dual interpolation now too: `gx-interp`, ms per replay).

Measured (the regression's input, the town walk right, 1280 x 720 OpenGL, 119 frames 30212-30330; the sideways offset
between the 4 rows above and below the join, sub-pixel): before, 30 fps setting: mean +0.7 px, swinging +-3.5 px
(std), 7.0 px from one frame to the next; before, 60 fps setting: +0.6, +-3.6, 6.9; after: +0.2, +-0.1, 0.1 (the
+0.2 is the scene's own slant there). Each part's own motion from frame to frame: before, 7.5-8 px every other frame
and 0 between; after, 3.3-4.4 px every frame for both (the near ground faster than the far shops), with no frame
apart. Cost: one replay of the new scene's geometry per frame on the emulation thread (0.5-0.7 ms in the town), and
the other screen's real scene drawn and split once more per frame on the GPU (the interpolated setting drew it
already). The game is unchanged: rendering only, OpenGL only; the software renderer and 4:3 never reach it (the
quick regression passes).

Pictures and composites in /home/claude/run/pcv4/: comp_join_wall_30230.png and comp_join_shop_30300.png (four
frames in a row at the join, 2x, three rows: before at 30 fps, before at 60 fps, after), comp_full_30231.png (whole
pictures), offset_per_frame.png (the join's offset per frame); the runs: longA (before, 30 fps), longB (before,
60 fps), long2 (after), w30210-w30345.

## The remaining screens, part 2 (2026-10-27)

Gone through by real input where it was quick (Harrows' furniture and CD orders, the post office's telegram, Mon
Paris, the classroom's homeroom and lesson, fishing by the bookshop) and the shops' own events otherwise (`MQ_EVENT_FILE`
21281 The Emporium's sell list, 21850 the hairstyles, both also in place for the hairstyles), OpenGL 1280 x 720 from
day1done.sav. Pictures and labelled composites in /home/claude/run/pcv5/ (outside the repo).

| screen (most met first) | what the game draws | PC view before | now | pictures |
| --- | --- | --- | --- | --- |
| talks with choices (Thom's "Talk / Write a telegram / Nothing", Harrows' "Buy / Don't buy", a lesson's question) | 3D room, 2D box and choices | the text band, choices inside it | unchanged | ls_8.png, l_menu.png |
| a menu with no text box (the post office's "Other player / Classmate / Exit"; an item's "Place / Bury / Release insect" is the same kind) | 2D window over the 3D | the touch screen at the zoomed scale (3.75x at 720p) | the window alone at the whole-number scale (2x) at the bottom middle, clicked there | comp_panel.png |
| lessons (homeroom, the tools lesson: the board, a practical, a question) | 3D classroom on the touch screen, the blackboard (2D) on the top | the room fills the window, text band, the board in the corner picture (large while it changes) | unchanged | comp_class.png |
| Harrows' furniture (A in front of an item: Piquard's "Would you like to purchase that?", price, Buy / Don't buy) | talk over the 3D room (3D on both screens: the room and the gallery above) | the room alone, text band | unchanged | g_cd_after.png |
| Harrows' catalogue: the CD order ("What song would you like to order?", ov037 "Music Learned") | the room's 3D moved to the top screen, the list in 2D below | the modal (3D on the top screen only) | unchanged; its backdrop clean | comp_shops.png |
| The Emporium's sell list (ov005), Mon Paris' hairstyles (ov014, the student in the chair above) | 3D on top, 2D list below | the modal | unchanged | comp_shops.png |
| the telegram's recipients (ov009 WHO) | 3D clerk on top, 2D grid | the modal, over a backdrop with the curtain that came in before it | the backdrop is the room as it was before the talk | comp_backdrop.png |
| fishing (the field, the fishing camera; the game hides the HUD) | 3D on both screens | joined; a band in a hair's colours from the window's edge, the HUD corners copied with a balloon's piece | fixed, see "Fishing" below | comp_fishing.png; pcv6/ |
| the bell's piano | 3D room on top, the composer in 2D | the modal, PC keys play it | (done 2026-10-26) | |
| the tournament's words (ov034 / battle HUD ov019-021) | 3D duel on top with the word slots, the tiles and the type gems in 2D below | the modal (3D on the top screen only); not pictured in the PC view this pass | | live/shots/hall/d3_begin.png (co-op, stacked) |
| insects, the hotpot's mushrooms, detention's cleaning | the field / a room in 3D | joined / the room alone (3D action) | not reached | |

Overlays with screens of their own (strings): ov005 sell list, ov006 design, ov007 closet, ov008 bank, ov009 telegram
recipients, ov010 insect and fish catalogues (the library), ov011 music player, ov012 UMA catalogue, ov013 the bell,
ov014 hairstyles, ov015 stamps, ov016 message board, ov019-021 the tournament's battle HUD, ov030-035 the Start menu's
pages and the keyboard, ov037 CD orders, ov038 the lesson pick. Everything 2D on the touch screen with 3D on the top
screen or none is the modal.

**A window with no text box.** The probe of the touch screen's 2D (64 x 48) also finds the longest run of rows drawn
opaque (alpha >= 200) across at least 24 points (96 DS pixels; in the HUD's rows only between its corners), at least
8 rows (32 DS rows) tall, and its columns (`bottom_overlay_panel*`). While the touch screen's world is shown, with no
menu, fade or text band, a window held 12 frames with the same rows is drawn crisp at the whole-number scale at the
bottom middle (`set_pc_panel`, a 4-row and 8-column margin; its columns are the widest seen while its rows hold: the
menu's cursor moves, and with the columns in the key the window went away for 12 frames at every move), clicked
there (checked: "Classmate" at window (550, 630) -> DS (125, 100), the recipients' grid opened; "Exit" closed it).
Name tags (20 rows) and thought balloons (60 pixels wide) are smaller than a window.

**The modal's backdrop.** The picture from before a menu opened caught what the touch screen had over the world as it
opened: the curtain coming in (the recipients' grid: a scalloped black band over the lower quarter), or the text
band and the window of the talk that opened it. A picture is no longer taken as the backdrop while the probe sees rows
drawn opaque across 48 of 64 points in the touch screen's top 6 or bottom 8 probe rows (`bottom_busy_`), or a text
band or window is laid out.

**Fishing (fixed 2026-10-27).** With the rod cast from the bookshop's bank (TESTING.md "Fishing by pad"), the picture
showed a band about 25 rows tall (y 360-385 at 720p) from the window's left edge to the student's head, and from an
edge to a classmate's hair (drawn over a classmate standing there), in the hair's colours; not in the DS's own picture,
not after walking on. It needs someone standing at the side of the 4:3 view and the snowy morning (2000-01-18 10:00;
at 09:30 and 14:00 the same walk showed none).

- **Not the renderer's fill, the sky bands, the stable textures or the interpolation:** with `display.fill_edges=false`
  and with `graphics.stable_textures=false` (live `set`) it stays; the band is one flat colour (188, 60, 52 across
  its 330 px), not the fill's blurred, darkened continuation; no polygon the game sends reaches the edge (a research
  log of every polygon wider than 100 px and flatter than 30 rows found only walls and ground).
- **The world extension** (world_ext.cpp, always on in the PC view): its polygons (polygon ID 62) included one with
  the hair's texture (0x0C030170), x 0 to about 135 of the 496-pixel wide picture, rows 44-67: the top of the
  student's hair, a level, upward-facing, solid, textured polygon reaching the view's edge, was taken for a row of the
  ground and carried on sideways to the window's edge (`MQ_LOG_WORLD`: "row x 40.15 y 0.40 z 72.93 edge 0.14 area
  0.016 tex 0c030170"). Its test against the fitted ground height (a row more than 0.35 above it is dropped) should
  have dropped it, but the fit was off by tens to hundreds of units ("above" -66 to +1600 for real ground rows): the
  level surfaces it is fitted to included the sky's two level backdrops, about 25000 square units each and 130-150
  units below the camera's point, which outweighed the ground by area (`MQ_LOG_WORLD_FIT`, new: the fit's points and
  rounds). So rows were kept and dropped at random, the hair among those kept (intermittent: it changes with what is
  in view). The walk's one-pixel orange line (regs/walkB/w32600) is the same, a classmate's hair at the view's side,
  and so was a straight band of cobbles above it (a raised edge carried on).
- **Fix:** surfaces over 400 square units are left out of the fit (the town's ground pieces are at most a cell, 16-20),
  z is taken about the surfaces' area-weighted middle (better conditioned), and a speck (area under 0.05 and an outer
  edge under 0.3 units: a hair's or a hat's top) is never a row. Fishing, the ground rows now sit within -1.8 to +0.5
  of the fit (spread 0.67) and the extension still fills the sides.

**The HUD while fishing.** The game takes the bag and the hand away while the rod is out, but the corner pieces still
copied DS (0, 0, 40, 40) and (216, 0, 40, 40): a speech balloon reaching those rectangles showed a piece of itself in
the window's lower left (and lost it where it was, the corner being a hole in the touch screen's 2D). The bag and the
hand (or the tool held) are 32 x 32 sprites of engine A at (0, 0) and (224, 2) (balloons are 16 x 16 and 32 x 16
pieces, palette 3 and 6); `mq_pcv_hud_read` (in mq_frame_end, PC view only, only read) looks for a shown 32 x 32
sprite at y <= 8 and x <= 8 / 216-224 in engine A's OAM on frames where engine A shows the touch screen (POWCNT1 bit
15 clear), and the presenter copies only those corners (`cfie::pc_view_hud_corners`, `set_pc_hud` takes a mask). In
the classroom the hand appears only with someone to talk to; the same check applies there (not pictured).

Pictures (1280 x 720 OpenGL llvmpipe; before: the build of 82b7f32) and labelled composites in /home/claude/run/pcv6/
(outside the repo): comp_fishing_band.png (fishing at 10:00, the band in 7 of 10 frames 15 apart before, 0 of 10 after,
and 0 of 10 in a second run), comp_walk_w32600.png, comp_walk_w32600_zoom.png and comp_walk_frames.png (the
regression's input, frames 32560-32700: a streak to the edge in each of the 8 shots before (two of them dotted), none after), comp_hud_fishing.png (the balloon's
piece in the lower left before; the corners empty while fishing after, the bag and hand back after it; the log's
`hud 3 -> 0` at the cast). The quick regression passes.

Testing: the live control file now takes `present-every <n>` and `draw <n>` (NTR_DRAW_EVERY=-1: the next n frames
drawn): reach a scene with the software renderer and no presents, then `set graphics.renderer=opengl`,
`present-every 1`, `draw 300` so the PC view's probe settles before the shot. A text box already written before the
switch to OpenGL is not seen as a text box (its draws were counted before): switch before the talk.

## The remaining screens, part 3 (2026-10-27)

The screens a player meets often that had not been looked at in the PC view, by real input (pad, the mouse through
the layout: the live control's `click`), OpenGL 1280 x 720 (llvmpipe), from a save made at the Gateway
(day1done.sav's student; clocks 2000-01-18 09:30, 2000-07-18 14:20 for insects, 2000-05-21 15:30 for the tournament).
Pictures and labelled composites in /home/claude/run/pcv7/ (outside the repo; the single pictures in
/home/claude/run/pv7/). Testing switch: `MQ_PCV_OLD_FILE=<path>`, while that file exists everything below acts as
before (the "before" of the composites, same moment).

| screen (most met first) | what the game draws | PC view before | now | mouse | pictures |
| --- | --- | --- | --- | --- | --- |
| the Gateway's menu and its save prompt ("Where would you like to go?", "Do you want to save your progress...", Yes / No) | 3D room, 2D box and choices | the text band, choices inside | unchanged | choices clicked (Save, Yes) | comp_save_prompt.png |
| saving ("Saving. Please wait...", "Save complete. Returning to the title screen...") | no 3D: a black top screen, the box alone at the bottom of a black touch screen | the modal: two black panes, the box in the lower part of one | the box alone at the bottom middle, where the text box was, crisp 2x, its black left out, over the dimmed room | (nothing to click) | comp_saving.png (sv9_*.png) |
| the cafe (sitting at the counter, Marcy's "Would you like something to drink?", "Today's special / Exit", the price) | 3D room, 2D box and choices | the text band | unchanged | "Today's special" clicked | comp_cafe.png |
| a speech balloon beside the HUD (the cafe's "Oh hohoho...!") | a 2D balloon at the touch screen's top left | a piece of the balloon copied with the bag into the window's lower left corner | the corners copy only the bag's and the hand's 32 x 32 sprites | | c2.png (before) |
| the bookshop's catalogues (the Insect Catalog's talk, Register: the bag) | the room in 3D, then the bag over the touch screen | the text band; the bag modal | unchanged | | comp_bookshop.png |
| insects with the net (a click on a cicada on a trunk) | the field | a click's touch went through the game's widened picking: the student walked several units past it (to the river) | clicks on the touch screen's world are mapped to the game's picking (below): the student turns to the trunk and swings at it | yes | comp_click_world.png |
| the wizard form's tip, the words screen (Y, or after the tip) | 3D on the top screen (the student) with the word slots, the tiles in 2D below | the modal | unchanged | tiles double-clicked, the DO tab, OK: three words chanted by mouse | comp_words_screen_mouse.png |
| the chant (the words above the student) | 2D sprites at the bottom of the top screen, over the student | in the top-right corner panel, small, far from the student | over the world where the DS has them (the top screen's 2D over its own part) | | comp_words_chant.png |
| the tournament's words (round one, the auditorium) | 3D duel on top with the slots, the tiles in 2D below | the modal over the picture kept from the town's gathering (its HUD and all), or black | over the auditorium's own 3D (below) | a tile double-clicked into the first slot | comp_tournament.png |
| the Start menu's Wizard ID and town map | 3D portrait on top, 2D pages below | the modal | unchanged | the map tab clicked | comp_start_menu.png |
| the title after a save ("Touch the screen!") | 3D on both screens | the touch screen alone (zoomed) | unchanged | | s_h2.png |
| "You caught ...!" | the field, a text box | | not reached: the insects reached by walking flew off (a catch by real input is in docs/COOP.md "Insects together") | | |
| detention's cleaning | maps 68-69 | | not reached: day1done.sav loaded without the visit marker is forgiven on its first offence ("But I spoke to Principal Sol..."), no cleaning | | |
| the hotpot cook-off's mushrooms, lesson minigames | | | not reached (the hotpot is in October's festival; the lessons' questions are talks with choices, the text band) | | |

**The box alone (saving).** A probe of the touch screen's whole picture (64 x 48, besides the 16 x 12 per screen)
finds the lowest row from which every row down is lit while every row above is black (`bottom_lit_band`). With the
modal up (a menu, or no 3D), the top screen black and such a box from DS row 64 down, the modal shows only those
rows (from 4 rows above) at the bottom middle, at the text box's scale, black pixels left out (`key_black`, a discard
in the screens' shader), over the dimmed picture from before (`set_pc_modal_show(2, band)`); held 8 frames, its top
only rising. With one screen black and the other not, the modal shows the lit one alone in the middle (1, 3). Only
with the renderer's parts (OpenGL; in software the probe does not run).

**The HUD's exact sprites.** The corners copied are the bag's 32 x 32 sprite at (0, 0) and the hand's at (224, 2),
not 40 x 40 rectangles, so a balloon beside them stays where it is (the cafe's "Oh hohoho...!" left a white piece
beside the bag in the window's corner).

**The top screen's 2D over its world.** Joined, the top screen's 2D is drawn over its own part (as the touch screen's
over its): the chanted words appear above the student, a classmate's "?" balloon over them; the corner panel shows
only the rows above that part (none of them is in the window). A curtain closing over both screens now covers the
top part too.

**World clicks through the game's picking.** The game's stylus picking uses its projection, widened with the picture
(cfie_hooks.cpp `mq_perspective_aspect`): a touch at x 0-255 is the whole wide picture's width, not the DS's middle.
A click on the touch screen's world (its whole picture, not a part drawn elsewhere, not a modal) where its 2D draws
nothing (the probe's 64 x 48 alpha, 4 DS pixels a point) is sent as `(x + e) * 256 / (256 + 2e)` (e: the wide
extra on each side; `pc_world_x`), and clicks in the wide sides are touches too now. Checked: with the net, a click on
a cicada on a trunk (window 447, 470) went to DS 54 and the student walked off to the river past the tree; now DS 82,
the student turns to the trunk and swings. A tap near the student does not walk (the game's own rule: DS 189 from
(128, 140) stays, 230 walks).

**A modal's backdrop from another area.** The picture kept as the modal's backdrop carries the area the field showed
(its +0x3EB4, `cfie::pc_view_area`, read at the frame's end); a flat modal (3D on the top screen only: the
tournament's words, reached from the town's gathering) whose kept picture is of another area, or none, shows the top
screen's world (the layer split's, without its 2D) filling the window, darkened (0.3), instead of the town. The bag
and the Start menu keep the kept picture (their top screen is the Wizard ID portrait).

Left: the modal's backdrop still shows the HUD corners of the picture it was kept from (dimmed); rooms' wide sides are
the renderer's stretched fill (the bookshop's shelves); clicks in the world beside a 2D element closer than 4 DS
pixels go to the 2D's coordinates.
