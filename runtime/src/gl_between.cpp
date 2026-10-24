#include "gl_between.hpp"

#include <memory>

#include "GPU.h"
#include "GPU2D_OpenGL.h"
#include "GPU_OpenGL.h"
#include "gx_hook.hpp"

namespace ntr {

namespace {

// Access to the GL renderer's private parts, through explicit template instantiation (which may name private
// members): the 3D output texture, the final pass and its framebuffers, and engine A's 2D compositor. melonDS's own
// sources are left as they are.
template <typename Tag, typename Tag::type M>
struct Grant {
    friend typename Tag::type get(Tag) { return M; }
};

struct Output3D {
    using type = GLuint melonDS::GLRenderer::*;
    friend type get(Output3D);
};
struct OutputFBs {
    using type = GLuint (melonDS::GLRenderer::*)[2];
    friend type get(OutputFBs);
};
struct BackBuffer {
    using type = int melonDS::Renderer::*;
    friend type get(BackBuffer);
};
struct Engine2DA {
    using type = std::unique_ptr<melonDS::Renderer2D> melonDS::Renderer::*;
    friend type get(Engine2DA);
};
struct FinalPass {
    using type = void (melonDS::GLRenderer::*)(int, int);
    friend type get(FinalPass);
};
struct Composite2D {
    using type = void (melonDS::GLRenderer2D::*)(int, int);
    friend type get(Composite2D);
};

// (the PC view's layers: engine A's compositor output, which the final pass reads, and the final output's size)
struct Output2DFB {
    using type = GLuint melonDS::GLRenderer2D::*;
    friend type get(Output2DFB);
};
struct Output2DTex {
    using type = GLuint (melonDS::GLRenderer::*)[2];
    friend type get(Output2DTex);
};
struct FinalTex {
    using type = GLuint (melonDS::GLRenderer::*)[2];
    friend type get(FinalTex);
};

template struct Grant<Output3D, &melonDS::GLRenderer::OutputTex3D>;
template struct Grant<Output2DFB, &melonDS::GLRenderer2D::OutputFB>;
template struct Grant<Output2DTex, &melonDS::GLRenderer::OutputTex2D>;
template struct Grant<FinalTex, &melonDS::GLRenderer::FPOutputTex>;
template struct Grant<OutputFBs, &melonDS::GLRenderer::FPOutputFB>;
template struct Grant<BackBuffer, &melonDS::Renderer::BackBuffer>;
template struct Grant<Engine2DA, &melonDS::Renderer::Rend2D_A>;
template struct Grant<FinalPass, &melonDS::GLRenderer::RenderScreen>;
template struct Grant<Composite2D, &melonDS::GLRenderer2D::RenderScreen>;

void copy_texture(GLuint read_fb, GLuint draw_fb, GLuint from, GLuint to, int w, int h) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fb);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, from, 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fb);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, to, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glDisable(GL_SCISSOR_TEST);
    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}  // namespace

GlBetween::~GlBetween() { release(); }

void GlBetween::release() {
    GLuint textures[2] = {saved_3d_, out_tex_};
    GLuint fbs[3] = {out_fb_, read_fb_, draw_fb_};
    if (saved_3d_ || out_tex_) glDeleteTextures(2, textures);
    if (out_fb_ || read_fb_ || draw_fb_) glDeleteFramebuffers(3, fbs);
    saved_3d_ = out_tex_ = out_fb_ = read_fb_ = draw_fb_ = 0;
    w_ = h_ = 0;
}

void GlBetween::ensure_size(int w, int h) {
    if (w == w_ && h == h_) return;
    release();
    w_ = w;
    h_ = h;
    GLuint t[2], f[3];
    glGenTextures(2, t);
    glGenFramebuffers(3, f);
    saved_3d_ = t[0];
    out_tex_ = t[1];
    out_fb_ = f[0];
    read_fb_ = f[1];
    draw_fb_ = f[2];
    glBindTexture(GL_TEXTURE_2D, saved_3d_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Laid out like the renderer's final output: one layer per screen, written as two colour attachments.
    glBindTexture(GL_TEXTURE_2D_ARRAY, out_tex_);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, w, h, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, out_fb_);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, out_tex_, 0, 0);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, out_tex_, 0, 1);
    const GLenum attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

uint32_t GlBetween::render(melonDS::GLRenderer& r) {
    const GLuint tex3d = r.*get(Output3D{});
    GLint w = 0, h = 0;
    glBindTexture(GL_TEXTURE_2D, tex3d);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
    if (w <= 0 || h <= 0) return 0;
    ensure_size(w, h);

    copy_texture(read_fb_, draw_fb_, tex3d, saved_3d_, w, h);
    r.Start3DRendering();  // the 3D of the current render list, into the renderer's 3D output
    auto& engine_a = static_cast<melonDS::GLRenderer2D&>(*(r.*get(Engine2DA{})));
    (engine_a.*get(Composite2D{}))(0, 192);  // engine A's screen again, with that 3D
    // The final pass (screen order, master brightness) into our output instead of the renderer's back buffer.
    GLuint(&fbs)[2] = r.*get(OutputFBs{});
    const int back = r.*get(BackBuffer{});
    const GLuint own = fbs[back];
    fbs[back] = out_fb_;
    (r.*get(FinalPass{}))(0, 192);
    fbs[back] = own;
    copy_texture(read_fb_, draw_fb_, saved_3d_, tex3d, w, h);
    return out_tex_;
}

GlLayers::~GlLayers() { release(); }

void GlLayers::release() {
    GLuint textures[4] = {scratch_tex_, pass_tex_, world_tex_, overlay_tex_};
    GLuint fbs[4] = {scratch_fb_, pass_fb_, read_fb_, draw_fb_};
    if (scratch_tex_) glDeleteTextures(4, textures);
    if (scratch_fb_) glDeleteFramebuffers(4, fbs);
    scratch_tex_ = pass_tex_ = world_tex_ = overlay_tex_ = 0;
    scratch_fb_ = pass_fb_ = read_fb_ = draw_fb_ = 0;
    w_ = h_ = aw_ = ah_ = 0;
}

// (w, h: the final output's size; aw, ah: engine A's composite's, the same unless only engine B's is wide)
void GlLayers::ensure_size(int w, int h, int aw, int ah) {
    if (w == w_ && h == h_ && aw == aw_ && ah == ah_) return;
    release();
    w_ = w;
    h_ = h;
    aw_ = aw;
    ah_ = ah;
    GLuint t[4], f[4];
    glGenTextures(4, t);
    glGenFramebuffers(4, f);
    scratch_tex_ = t[0];
    pass_tex_ = t[1];
    world_tex_ = t[2];
    overlay_tex_ = t[3];
    scratch_fb_ = f[0];
    pass_fb_ = f[1];
    read_fb_ = f[2];
    draw_fb_ = f[3];
    // (filtered like engine A's own output, which the final pass samples)
    glBindTexture(GL_TEXTURE_2D, scratch_tex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, aw, ah, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, scratch_fb_);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, scratch_tex_, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    for (GLuint tex : {pass_tex_, world_tex_, overlay_tex_}) {
        glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, w, h, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    // (the 2D starts see-through and the world black, until each screen's first split)
    const GLenum attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glBindFramebuffer(GL_FRAMEBUFFER, pass_fb_);
    glDisable(GL_SCISSOR_TEST);
    for (GLuint tex : {world_tex_, overlay_tex_}) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0, 0);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tex, 0, 1);
        glDrawBuffers(2, attachments);
        glClearColor(0, 0, 0, tex == world_tex_ ? 1.0f : 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    // Laid out like the renderer's final output: one layer per screen, written as two colour attachments.
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pass_tex_, 0, 0);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, pass_tex_, 0, 1);
    glDrawBuffers(2, attachments);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

namespace {

// The GL state split() and ensure_size() change besides what melonDS's own passes set again: put back as it was, so
// that nothing drawn after (the display capture, the frontend) depends on what ran in between.
struct SavedGlState {
    GLint active = 0, tex2d = 0, array = 0, read_fb = 0, draw_fb = 0;
    GLfloat clear[4] = {0, 0, 0, 0};
    GLboolean scissor = GL_FALSE;
    SavedGlState() {
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex2d);
        glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &array);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fb);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fb);
        glGetFloatv(GL_COLOR_CLEAR_VALUE, clear);
        scissor = glIsEnabled(GL_SCISSOR_TEST);
    }
    ~SavedGlState() {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GLuint(tex2d));
        glBindTexture(GL_TEXTURE_2D_ARRAY, GLuint(array));
        glActiveTexture(GLenum(active));
        glBindFramebuffer(GL_READ_FRAMEBUFFER, GLuint(read_fb));
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(draw_fb));
        glClearColor(clear[0], clear[1], clear[2], clear[3]);
        if (scissor) glEnable(GL_SCISSOR_TEST);
        else glDisable(GL_SCISSOR_TEST);
    }
};

}  // namespace

bool GlLayers::split(melonDS::GLRenderer& r, int layer) {
    if (layer < 0 || layer > 1) return false;
    const SavedGlState saved;
    GLuint(&final_tex)[2] = r.*get(FinalTex{});
    GLuint(&tex2d)[2] = r.*get(Output2DTex{});
    const int back = r.*get(BackBuffer{});
    GLint w = 0, h = 0, aw = 0, ah = 0;
    glBindTexture(GL_TEXTURE_2D_ARRAY, final_tex[back]);
    glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_HEIGHT, &h);
    glBindTexture(GL_TEXTURE_2D, tex2d[0]);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &aw);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &ah);
    if (w <= 0 || h <= 0 || aw <= 0 || ah <= 0) return false;
    ensure_size(w, h, aw, ah);

    // Engine A's compositor and the final pass draw into ours instead of the renderer's (whose output the display
    // capture that follows reads), then everything is put back.
    auto& engine_a = static_cast<melonDS::GLRenderer2D&>(*(r.*get(Engine2DA{})));
    GLuint& a_fb = engine_a.*get(Output2DFB{});
    GLuint(&fbs)[2] = r.*get(OutputFBs{});
    const GLuint own_a_fb = a_fb, own_a_tex = tex2d[0], own_out = fbs[back];
    a_fb = scratch_fb_;
    tex2d[0] = scratch_tex_;
    fbs[back] = pass_fb_;
    for (int part = 1; part <= 2; part++) {
        melonDS::FfccLayerSplit = part;
        (engine_a.*get(Composite2D{}))(0, 192);
        (r.*get(FinalPass{}))(0, 192);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fb_);
        glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pass_tex_, 0, layer);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fb_);
        glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, part == 1 ? world_tex_ : overlay_tex_, 0, layer);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glDisable(GL_SCISSOR_TEST);
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    melonDS::FfccLayerSplit = 0;
    a_fb = own_a_fb;
    tex2d[0] = own_a_tex;
    fbs[back] = own_out;
    return true;
}

}  // namespace ntr
