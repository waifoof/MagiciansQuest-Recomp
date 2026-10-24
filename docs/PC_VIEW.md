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

1. Single-camera field render (research running): one projection covering both screens' windows plus the extra
   width, rendered every frame; or, failing that, the two halves rendered from one eye without a seam.
2. Layer split in the renderer (3D alone, 2D overlay with alpha) by re-running melonDS's 2D compositor the way
   `runtime/src/gl_between.cpp` does.
3. Presenter: a new `Layout` value (appended last), more views than two, panels with their own crop rects, touch
   mapping through each panel.
4. Scene detection from game hooks (text box, menu open/close, HUD live) rather than framebuffer guessing.
5. Settings UI rows; reviewers (agents) go over each scene for inconsistencies.
