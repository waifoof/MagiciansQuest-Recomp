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
   hooks write the game's memory, and every console of a session must run the same game; the layout then still
   arranges the pictures, from the game's own two cameras.
2. Layer split in the renderer (written, not yet run): at the VBlank, engine A's screen is composited twice more
   (`FfccLayerSplit`: 1 the world, the layers up to the 3D's place in the priority order; 2 the 2D after it, with
   alpha), each through the final pass, into per-screen arrays (`GlLayers` in `runtime/src/gl_between.cpp`,
   `Frame::gl_world`/`gl_overlay`). OpenGL and compute only; the software renderer shows whole screens.
3. Presenter (written, not yet run): the `PcView` layout (`pc-view`). Joined: both screens' world stacked, the bottom
   screen's 2D over its part, the top screen's 2D as a corner panel (and over the top part during fades); 3D on the
   bottom only: as One screen's solo; no 3D: as its flat. Touch only through the bottom screen's part (`screen_at`).
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
  Risks: geometry the game streams only for its own views (pop-in
  above), fog stepping at the join, a one-frame tear at the join while the camera moves (each half is 30 fps).
