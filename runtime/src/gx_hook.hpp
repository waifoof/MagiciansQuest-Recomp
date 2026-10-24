// Declarations for our patches to melonDS's GPU3D.cpp and NDSCart.cpp (applied at configure time, see
// third_party/CMakeLists.txt).
#pragma once

#include "types.h"

namespace melonDS {
// Called for every GX command FIFO entry the geometry engine executes (not while replaying).
extern void (*FfccGxCommandHook)(void* context, u8 command, u32 param);
// Called at the end of every VBlank; `flushed` when it swapped in a new polygon list (not while replaying).
extern void (*FfccGxVBlankHook)(void* context, bool flushed);
extern void* FfccGxCommandHookContext;  // passed to both hooks
// While set, the geometry engine raises no GX FIFO interrupts or DMA requests (commands fed to it by the runtime).
extern bool FfccGxReplaying;
// Widescreen: extra pixels on each side of the top screen's picture, at 1x (0 = 4:3). The geometry engine widens a
// full-screen viewport by it, and the OpenGL renderer's 3D texture, engine A's 2D (sprites, composite) and the final
// output follow (with the 256 DS pixels in the middle). Set before the renderer is made.
extern int FfccWideExtra;
// Both screens wide: a game drawing 3D on both screens through display capture (third_party/CMakeLists.txt).
extern int FfccWideBoth;
// 3D on both screens at 60 fps (FfccWideBoth's kept pictures, with the real scene behind each blended frame drawn by
// FfccDualRealHook: see gx::Streams::dual).
extern int FfccDual60;
// The scene flushed last was drawn widened (perspective geometry with widescreen on).
extern int FfccWide3D;
extern int FfccSkipDraw;  // tests: this frame's pictures are not drawn (software renderer; NTR_DRAW_EVERY)
extern int FfccTexScale;  // texture packs (GPU3D_TexcacheOpenGL.cpp)
extern int (*FfccTexHook)(void*, unsigned, unsigned, const unsigned*, unsigned*, unsigned, unsigned);
extern void* FfccTexHookContext;
extern int FfccSmooth2D;   // the 2D layers and sprites upscaled with rounded diagonals (see the compositor)
extern int FfccFillHoles;  // the wide sides' empty parts continue the scene beside them (see the compositor)
extern int FfccBoxAll;  // debugging: every box test passes (MQ_BOXALL_AT=<frame>)
// The screen (0 top, 1 bottom) that showed engine A's live picture last VBlank (-1: mixed or unknown), and whether
// the screens took turns (3D on both). Set with FfccWideBoth or FfccDual60.
extern int FfccLiveLayer;
extern int FfccDualShown;
extern unsigned (*FfccDualRealHook)(void*);
extern void* FfccDualRealContext;
// The PC view's layers (GlLayers): FfccLayerSplitHook is called at the VBlank with the layer (0 top, 1 bottom) that
// showed engine A live, and composites it again with FfccLayerSplit set (1 the world, 2 the 2D in front of the 3D).
extern int FfccLayerSplit;
extern void (*FfccLayerSplitHook)(void* context, int layer);
extern void* FfccLayerSplitContext;
// The HUD (engine A's sprite clusters at the screen's side edges): moved to the edges of the wide picture when
// FfccHudEdges, and scaled by FfccHudScale percent about the screen's corner.
extern int FfccHudEdges, FfccHudScale;
// Stable texture mapping: quads with texture coordinates no perspective transform fits are cut into a grid.
extern int FfccStableUV;
// GPU3D_OpenGL.cpp: texture smoothing in the 3D shader (0 off, 1 smooth, 2 sharp).
extern int FfccSmoothTex;
// NDSCart.cpp: cartridge transfers without gaps, at 4 cycles a word.
extern bool FfccFastCart;
}  // namespace melonDS
