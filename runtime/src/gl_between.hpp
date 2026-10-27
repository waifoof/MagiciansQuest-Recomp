// Frames between emulated frames, for 3D at the display's refresh rate (docs/ARCHITECTURE.md §5.3): draws the 3D of
// the current render list again with melonDS's OpenGL or compute renderer, and composites both screens again into an
// output of its own, the way the last emulated frame did. The renderer's own 3D output, which the next emulated frame
// composites, is put back afterwards.
#pragma once

#include <cstdint>
#include <functional>

namespace melonDS {
class GLRenderer;
}

namespace ntr {

class GlBetween {
public:
    GlBetween() = default;
    GlBetween(const GlBetween&) = delete;
    GlBetween& operator=(const GlBetween&) = delete;
    ~GlBetween();  // needs the GL context current

    // Draws a frame from the current render list. Returns the output texture (a 2-layer array: the top and bottom
    // screens, like GLRenderer::GetFramebuffers), valid until the next call.
    // `with_3d`, if given, runs while that frame's 3D is the renderer's 3D output (before it is put back).
    uint32_t render(melonDS::GLRenderer& renderer, const std::function<void()>& with_3d = nullptr);

private:
    void ensure_size(int w, int h);
    void release();

    int w_ = 0, h_ = 0;
    uint32_t saved_3d_ = 0;   // the renderer's own 3D output, kept while the extra frame is drawn
    uint32_t out_tex_ = 0;    // the composited screens
    uint32_t out_fb_ = 0;
    uint32_t read_fb_ = 0, draw_fb_ = 0;  // for copying 3D outputs
};

// The PC view's layers (docs/PC_VIEW.md, step 2): each screen's picture in two parts, the world (the 3D and what is
// behind it) and the 2D in front of it, with alpha. melonDS composites engine A's screen twice more at the VBlank
// (FfccLayerSplit), each through the final pass so the master brightness (fades) applies, into that screen's layer
// of two arrays of our own. The game draws its 3D on the screens in turn, so each screen's layers are from the last
// frame that screen showed engine A live. The renderer's own output is left as it was.
class GlLayers {
public:
    GlLayers() = default;
    GlLayers(const GlLayers&) = delete;
    GlLayers& operator=(const GlLayers&) = delete;
    ~GlLayers();  // needs the GL context current

    // At the VBlank (FfccLayerSplitHook): `layer` (0 top, 1 bottom) is the screen engine A showed. False if nothing
    // was made (no output yet).
    bool split(melonDS::GLRenderer& renderer, int layer);
    // 3D on both screens interpolated (gx::Streams::dual, "One moment" in docs/PC_VIEW.md): the world of `layer` as
    // its real scene, while that is the renderer's 3D output (from GlBetween::render's with_3d), kept apart; and
    // use_real puts the one kept for `layer` in the world array, in place of what the screen showed (the blend).
    bool split_real(melonDS::GLRenderer& renderer, int layer);
    void use_real(int layer);
    // 2-layer arrays laid out like the renderer's output (GLRenderer::GetFramebuffers), 0 before the first split:
    // the world (opaque), and the 2D in front of it (premultiplied alpha).
    uint32_t world() const { return world_tex_; }
    uint32_t overlay() const { return overlay_tex_; }

private:
    void ensure_size(int w, int h, int aw, int ah);
    void release();
    bool split_parts(melonDS::GLRenderer& renderer, int layer, bool real);

    int w_ = 0, h_ = 0, aw_ = 0, ah_ = 0;
    uint32_t scratch_tex_ = 0, scratch_fb_ = 0;  // engine A's composite, in place of its own output
    uint32_t pass_tex_ = 0, pass_fb_ = 0;        // the final pass's two screens, in place of the renderer's
    uint32_t world_tex_ = 0, overlay_tex_ = 0;
    uint32_t real_tex_ = 0;  // split_real's worlds
    uint32_t read_fb_ = 0, draw_fb_ = 0;
};

}  // namespace ntr
