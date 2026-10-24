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
   ID portrait over its touch menu): both screens 4:3 side by side (One screen's flat). Touch only through the bottom
   screen's part (`screen_at`). World extension (world_ext.cpp) is always on in the PC view. The layer arrays sample
   with CLAMP_TO_EDGE (the seam at the join and the bright outer rows were GL_REPEAT wrapping).
4. Scene detection from game hooks (text box, menu open/close, HUD live) rather than framebuffer guessing.
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

Left:
- The wide sides still show what the renderer's fill makes of the game's 4:3 culling: objects cut at the old view's
  edge are stretched sideways (an NPC at the far left, the sky bands' far ends). World extension carries only the
  ground on. Widening the game's own object culling for the PC view is not done.
- Fades into and out of a scene: the game draws those frames with a projection of its own (not through the 0x1555
  setup the aspect hook widens), so they are 4:3 in the middle with black sides for a moment (seen in the classroom,
  bottom screen alone). The joined picture's sides are darkened with the fade's weight, not yet seen in a joined fade.
- HUD clusters at the window's lower corners (touch remapped with them), the dialogue box at an integer scale
  anchored bottom-centre, the menu modal and the eased "look up": not done. The HUD and text boxes are drawn at the
  bottom screen's scale (2.6x at 1280 x 720).
- The Start menu's arrangement (both screens side by side, from 3D on the top screen only) is written but was not
  seen running (Start did not open the menu in the OpenGL test session).
