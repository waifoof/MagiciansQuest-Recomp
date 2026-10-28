#include "video.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

#include "ntr/hash.hpp"
#include "OpenGLSupport.h"  // melonDS: glad and the shader compile helper
#ifdef _WIN32
#include "flip_present.hpp"
#endif

namespace video {

namespace {

constexpr int kScreenW = 256, kScreenH = 192;

const char* kVS = R"(#version 140
uniform vec4 uDest;  // x0, y0, x1, y1 in clip space (y0 = top edge)
uniform vec2 uUvX;   // the part of the texture's width the screen shows
uniform vec2 uUvY;   // ... and of its height
uniform float uLayer;
in vec2 vPos;        // unit quad, (0,0) = top left
out vec3 fTex;
void main() {
    gl_Position = vec4(mix(uDest.xy, uDest.zw, vPos), 0.0, 1.0);
    fTex = vec3(mix(uUvX.x, uUvX.y, vPos.x), mix(uUvY.x, uUvY.y, vPos.y), uLayer);
}
)";

// Sharp bilinear (after Themaister's shader): magnify each texel by the largest integer factor that fits and blend
// only across the remaining fraction of an output pixel at texel edges. Needs the texture's filter set to linear.
const char* kFS = R"(#version 140
uniform sampler2DArray uTex;
uniform int uSharp;
uniform vec2 uOutSize;  // the screen's size on the window, in pixels
uniform float uOpacity;
uniform int uAlpha;     // the picture's own alpha (premultiplied: the PC view's 2D), times uOpacity
uniform vec4 uHoles[4]; // (PC view) parts left out (u0, v0, u1, v1): drawn elsewhere
uniform int uHoleCount;
in vec3 fTex;
out vec4 oColor;
void main() {
    vec2 uv = fTex.xy;
    for (int i = 0; i < uHoleCount; i++)
        if (uv.x >= uHoles[i].x && uv.x < uHoles[i].z && uv.y >= uHoles[i].y && uv.y < uHoles[i].w) discard;
    if (uSharp != 0) {
        vec2 size = vec2(textureSize(uTex, 0).xy);
        vec2 texel = uv * size;
        vec2 scale = max(floor(uOutSize / size), vec2(1.0));
        vec2 d = fract(texel) - 0.5;
        vec2 range = 0.5 - 0.5 / scale;
        uv = (floor(texel) + (d - clamp(d, -range, range)) * scale + 0.5) / size;
    }
    // (a picture drawn larger than it is shown, supersampling: each output pixel averages the texels it covers)
    vec2 ratio = vec2(textureSize(uTex, 0).xy) / max(uOutSize, vec2(1.0));
    if (uSharp == 0 && max(ratio.x, ratio.y) > 1.25) {
        vec2 step = ratio / vec2(textureSize(uTex, 0).xy);
        int n = int(clamp(ceil(max(ratio.x, ratio.y) * 0.5), 1.0, 4.0));
        vec4 sum = vec4(0.0);
        for (int j = 0; j < n; j++)
            for (int i = 0; i < n; i++) {
                vec2 o = (vec2(float(i), float(j)) + 0.5) / float(n) - 0.5;
                sum += texture(uTex, vec3(uv + o * step, fTex.z));
            }
        sum /= float(n * n);
        oColor = uAlpha == 1 ? sum * uOpacity : vec4(sum.rgb, uOpacity);
        return;
    }
    vec4 c = texture(uTex, vec3(uv, fTex.z));
    if (uAlpha == 2 && max(c.r, max(c.g, c.b)) < 0.02) discard;  // (black left out: a box alone on a black screen)
    oColor = uAlpha == 1 ? c * uOpacity : vec4(c.rgb, uOpacity);
}
)";

}  // namespace

SDL_Surface* Presenter::read_back(unsigned target, int width, int height) {
    SDL_Surface* surf = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ABGR8888);
    if (!surf) return nullptr;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, target);
    glReadBuffer(target ? GL_COLOR_ATTACHMENT0 : GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glPixelStorei(GL_PACK_ROW_LENGTH, surf->pitch / 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    SDL_FlipSurface(surf, SDL_FLIP_VERTICAL);  // GL rows run bottom-up
    return surf;
}

void Presenter::save_png(const char* path, unsigned target, int width, int height) {
    SDL_Surface* surf = read_back(target, width, height);
    if (!surf) return;
    if (!SDL_SavePNG(surf, path)) std::fprintf(stderr, "cannot save %s: %s\n", path, SDL_GetError());
    SDL_DestroySurface(surf);
}

void Presenter::gpu_begin() {
    serial_++;
    if (!gpu_timing_ || gpu_active_ || !GLAD_GL_VERSION_3_3) return;
    GpuQuery q;
    q.serial = serial_;
    if (!gpu_free_.empty()) {
        q.id = gpu_free_.back();
        gpu_free_.pop_back();
    } else if (gpu_pending_.size() < 16) {
        glGenQueries(1, &q.id);
    } else {
        return;  // the results are not coming back: skip this one
    }
    glBeginQuery(GL_TIME_ELAPSED, q.id);
    gpu_pending_.push_back(q);
    gpu_active_ = true;
}

void Presenter::gpu_end() {
    if (!gpu_active_) return;
    glEndQuery(GL_TIME_ELAPSED);
    gpu_active_ = false;
}

bool Presenter::poll_gpu_time(std::uint64_t& serial, double& ms) {
    if (gpu_pending_.empty() || gpu_active_) return false;
    const GpuQuery q = gpu_pending_.front();
    GLint ready = 0;
    glGetQueryObjectiv(q.id, GL_QUERY_RESULT_AVAILABLE, &ready);
    if (!ready) return false;
    GLuint64 ns = 0;
    glGetQueryObjectui64v(q.id, GL_QUERY_RESULT, &ns);
    gpu_pending_.pop_front();
    gpu_free_.push_back(q.id);
    serial = q.serial;
    ms = double(ns) / 1e6;
    return true;
}

const char* layout_name(Layout l) {
    switch (l) {
    case Layout::Single: return "single screen";
    case Layout::MainSide: return "main + side";
    case Layout::Stacked: return "stacked";
    case Layout::SideBySide: return "side by side";
    case Layout::Windows: return "two windows";
    case Layout::OneScreen: return "one screen";
    case Layout::PcView: return "PC view";
    }
    return "?";
}

const char* align_name(Align a) {
    switch (a) {
    case Align::Top: return "top";
    case Align::Middle: return "middle";
    case Align::Bottom: return "bottom";
    }
    return "?";
}

const char* filter_name(Filter f) {
    switch (f) {
    case Filter::Nearest: return "nearest";
    case Filter::Linear: return "linear";
    case Filter::Sharp: return "sharp";
    }
    return "?";
}

#ifndef _WIN32
class FlipPresenter {};  // Windows only
#endif

Presenter::Presenter() = default;
Presenter::~Presenter() { shutdown(); }

bool Presenter::init(SDL_Window* window, bool allow_flip_model) {
    window_ = window;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    // 4.3 adds the compute shaders melonDS's compute renderer needs; 3.3 is enough for everything else, including the menu.
    for (auto [major, minor] : {std::pair{4, 3}, std::pair{3, 3}}) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
        if ((context_ = SDL_GL_CreateContext(window))) break;
    }
    if (!context_) {
        std::fprintf(stderr, "cannot create an OpenGL 3.3 context: %s\n", SDL_GetError());
        return false;
    }
    SDL_GL_MakeCurrent(window, static_cast<SDL_GLContext>(context_));
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        std::fprintf(stderr, "cannot load the OpenGL functions\n");
        return false;
    }
    gl43_ = GLAD_GL_VERSION_4_3 != 0;
    SDL_GL_SetSwapInterval(0);  // the frontend paces frames to the DS refresh rate itself
    std::printf("OpenGL %s (%s)\n", reinterpret_cast<const char*>(glGetString(GL_VERSION)),
                reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    GLuint prog = 0;
    if (!melonDS::OpenGL::CompileVertexFragmentProgram(prog, kVS, kFS, "ScreenShader", {{"vPos", 0}}, {{"oColor", 0}}))
        return false;
    program_ = prog;
    u_dest_ = glGetUniformLocation(prog, "uDest");
    u_layer_ = glGetUniformLocation(prog, "uLayer");
    u_sharp_ = glGetUniformLocation(prog, "uSharp");
    u_out_size_ = glGetUniformLocation(prog, "uOutSize");
    u_tex_ = glGetUniformLocation(prog, "uTex");
    u_opacity_ = glGetUniformLocation(prog, "uOpacity");
    u_uv_x_ = glGetUniformLocation(prog, "uUvX");
    u_uv_y_ = glGetUniformLocation(prog, "uUvY");
    u_alpha_ = glGetUniformLocation(prog, "uAlpha");
    u_holes_ = glGetUniformLocation(prog, "uHoles");
    u_hole_count_ = glGetUniformLocation(prog, "uHoleCount");

    const float quad[] = {0, 0, 1, 0, 0, 1, 1, 1};  // triangle strip
    GLuint b, a, t;
    glGenBuffers(1, &b);
    glBindBuffer(GL_ARRAY_BUFFER, b);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glGenVertexArrays(1, &a);
    glBindVertexArray(a);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    vbo_ = b;
    vao_ = a;

    // The software renderer's two screens are uploaded into the same kind of array texture the GL renderers output.
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D_ARRAY, t);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, kScreenW, kScreenH, 2, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    soft_tex_ = t;

#ifdef _WIN32
    if (allow_flip_model) {
        void* hwnd = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        auto flip = std::make_unique<FlipPresenter>();
        std::string error;
        if (hwnd && flip->init(hwnd, error)) {
            flip_ = std::move(flip);
            std::printf("presentation: DXGI flip model\n");
        } else {
            std::printf("presentation: OpenGL swap (flip model unavailable: %s)\n", error.c_str());
        }
    } else {
        std::printf("presentation: OpenGL swap\n");
    }
#else
    (void)allow_flip_model;
#endif
    return true;
}

void Presenter::set_picture(const std::uint8_t* rgb, int w, int h, std::uint64_t serial, const Rect& ds) {
    picture_ds_ = ds;
    picture_on_ = rgb && w > 0 && h > 0;
    if (!picture_on_ || serial == picture_serial_) return;
    if (!picture_tex_) {
        GLuint t;
        glGenTextures(1, &t);
        picture_tex_ = t;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, picture_tex_);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (w != picture_w_ || h != picture_h_) {
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, w, h, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        picture_w_ = w;
        picture_h_ = h;
    } else {
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, w, h, 1, GL_RGB, GL_UNSIGNED_BYTE, rgb);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    picture_serial_ = serial;
}

void Presenter::shutdown() {
    if (!context_) return;
    close_second();
    flip_.reset();
    if (probe_fence_) glDeleteSync(static_cast<GLsync>(probe_fence_));
    probe_fence_ = nullptr;
    if (probe_tex_) {
        glDeleteFramebuffers(2, probe_fbo_);
        glDeleteTextures(1, &probe_tex_);
        glDeleteBuffers(1, &probe_pbo_);
        probe_tex_ = probe_pbo_ = probe_fbo_[0] = probe_fbo_[1] = 0;
    }
    if (backdrop_tex_) {
        GLuint bt = backdrop_tex_, bf = backdrop_fbo_;
        glDeleteTextures(1, &bt);
        glDeleteFramebuffers(1, &bf);
        backdrop_tex_ = backdrop_fbo_ = 0;
        backdrop_w_ = backdrop_h_ = 0;
        backdrop_ok_ = false;
    }
    GLuint t = soft_tex_, b = vbo_, a = vao_;
    if (picture_tex_) {
        GLuint p = picture_tex_;
        glDeleteTextures(1, &p);
        picture_tex_ = 0;
        picture_w_ = picture_h_ = 0;
    }
    if (t) glDeleteTextures(1, &t);
    if (b) glDeleteBuffers(1, &b);
    if (a) glDeleteVertexArrays(1, &a);
    if (program_) glDeleteProgram(program_);
    SDL_GL_DestroyContext(static_cast<SDL_GLContext>(context_));
    context_ = nullptr;
}

void Presenter::layout(int w, int h, const LayoutSettings& s) {
    // One screen: a new arrangement (3D now on one screen, a menu, the picture-in-picture's size) is moved to over a
    // moment, from where the screens are shown now.
    ScreenView before[kMaxViews];
    const int before_count = current_views(before);
    const bool one = joins_screens(s.layout);
    const bool moved = one && before_count >= 2 && laid_one_ &&
                       (one_mode_ != laid_mode_ || pip_ != laid_pip_ || pip_attention_ != laid_attention_ ||
                        solo_screen_ != laid_solo_ || pc_modal_ != laid_modal_ || pc_modal_show_ != laid_show_);
    laid_one_ = one;
    laid_mode_ = one_mode_;
    laid_pip_ = pip_;
    laid_attention_ = pip_attention_;
    laid_solo_ = solo_screen_;
    laid_modal_ = pc_modal_;
    laid_show_ = pc_modal_show_;
    layout_views(w, h, s);
    if (moved) {
        for (int i = 0; i < before_count; i++) anim_from_[i] = before[i];
        anim_from_count_ = before_count;
        anim_start_ = SDL_GetTicks();
    }
}

// The views as they are shown now: views_, or on the way to them from anim_from_ (eased, by screen).
int Presenter::current_views(ScreenView* out) const {
    for (int i = 0; i < view_count_; i++) out[i] = views_[i];
    if (!anim_start_ || !anim_from_count_) return view_count_;
    const float t = std::min(1.0f, (SDL_GetTicks() - anim_start_) / 260.0f);
    if (t >= 1.0f) return view_count_;
    const float e = t * t * (3 - 2 * t);
    for (int i = 0; i < view_count_; i++) {
        // (a screen's whole picture or its world moves to its world's place, its 2D to its 2D's; a 2D that was not shown
        // before starts from where its screen was)
        const ScreenView* from = nullptr;
        const ScreenView* screen_from = nullptr;
        const bool overlay = views_[i].picture == Picture::Overlay;
        for (int j = 0; j < anim_from_count_; j++) {
            if (anim_from_[j].screen != views_[i].screen) continue;
            const bool was_overlay = anim_from_[j].picture == Picture::Overlay;
            if (was_overlay == overlay) from = &anim_from_[j];
            if (!was_overlay && !screen_from) screen_from = &anim_from_[j];
        }
        if (!from && overlay) from = screen_from;
        if (!from) continue;
        auto mix = [e](float a, float b) { return a + (b - a) * e; };
        const Rect& a = from->rect;
        const Rect& b = views_[i].rect;
        out[i].rect = {mix(a.x, b.x), mix(a.y, b.y), mix(a.w, b.w), mix(a.h, b.h)};
        out[i].shown = mix(shown_width(*from), shown_width(views_[i]));
        out[i].opacity = mix(from->opacity, views_[i].opacity);
        out[i].framed = views_[i].framed || from->framed;
    }
    return view_count_;
}

// One screen, 3D on one screen only: that screen fills the window (as much of its wide picture as the window's shape
// shows), and the other is a 4:3 picture-in-picture in the top right corner (small, large, or moved off the window).
void Presenter::solo_layout(int w, int h) {
    const int main = solo_screen_, other = 1 - main;
    const float full = (main == 0 || wide_both_) ? float(kScreenW + 2 * wide_extra_) : float(kScreenW);
    const float shown = std::clamp(kScreenH * float(w) / std::max(1, h), float(kScreenW), std::max(full, float(kScreenW)));
    const float aspect = shown / kScreenH;
    const float mh = std::min(float(h), w / aspect), mw = mh * aspect;
    const Rect r{std::round((w - mw) / 2), std::round((h - mh) / 2), mw, mh};
    views_[0] = {main, r, 1.0f, false, shown};
    int size = pip_;
    if (pip_attention_) size = pip_ == 2 ? 0 : 1;
    const float ph = std::round(h * (size == 1 ? 0.42f : 0.25f)), pw = std::round(ph * kScreenW / kScreenH);
    const float m = std::round(h * 0.02f);
    float x = size == 2 ? float(w) + m : r.x + r.w - pw - m;
    float py = r.y + m, pw2 = pw, ph2 = ph;
    // (PC view: the touch screen's DS rectangle is the middle of the picture, its corners hold buttons (the classroom's
    // talk button): the corner picture goes in the margin beside it, as large as the margin allows, and only while it
    // is asked to be large (new text) over the rectangle; clicks on it reach the touch screen anyway, see screen_at)
    if (pc_view_ && main == 1 && size != 2 && !pip_attention_) {
        const float ds_w = r.w * kScreenW / std::max(shown, float(kScreenW));
        const float margin = (r.w - ds_w) / 2 - 2 * m;
        if (margin > 0) {
            pw2 = std::min(pw, margin);
            ph2 = std::round(pw2 * kScreenH / kScreenW);
            pw2 = std::round(pw2);
            x = r.x + r.w - pw2 - m;
        }
    }
    views_[1] = {other, {x, py, pw2, ph2}, 1.0f, true, float(kScreenW)};
    view_count_ = 2;
    // (PC view, the touch screen alone: its world, its 2D over it, the 2D's parts that have places of their own, and
    // the corner picture last)
    if (pc_view_ && main == 1) {
        const ScreenView panel = views_[1];
        views_[0].picture = Picture::World;
        views_[1] = views_[0];
        views_[1].picture = Picture::Overlay;
        view_count_ = 2;
        pc_parts(w, h, r);
        views_[view_count_++] = panel;
    }
}

// PC view: the touch screen's 2D parts with places of their own (set_pc_hud, set_pc_text), added after its whole 2D
// (`bottom`: where that is drawn), which leaves them out (holes_).
void Presenter::pc_parts(int w, int h, const Rect& bottom) {
    (void)bottom;
    hole_count_ = 0;
    const int k = std::max(1, std::min(pc_scale(h), w / kScreenW));
    const float m = std::round(h * 0.015f);
    auto piece = [&](const Rect& src, const Rect& dst) {
        if (view_count_ >= kMaxViews - 1 || hole_count_ >= 4) return;
        ScreenView v;
        v.screen = 1;
        v.rect = dst;
        v.shown = float(kScreenW);
        v.picture = Picture::Overlay;
        v.src = src;
        v.crisp = true;
        views_[view_count_++] = v;
        holes_[hole_count_++] = src;
    };
    // (the HUD's rows are inside a text box's band that reaches them: drawn there, in place, not twice)
    constexpr float kHud = 40;
    const bool text_top = pc_text_bottom_ >= 0, text_high = pc_text_top_ >= 0 && pc_text_top_ < kHud;
    // (a window alone, set_pc_panel: its rows at the bottom middle, as the text box's; not with a text box)
    // (the corners are the HUD's sprites exactly, the bag 32 x 32 at (0, 0), the hand 32 x 32 at (224, 2): a speech
    // balloon beside them stays where it is, rather than a piece of it going along to the window's corner)
    auto hud_pieces = [&] {
        if (pc_old_) {
            const float s = kHud * k;
            if (pc_hud_ & 1) piece({0, 0, kHud, kHud}, {m, h - s - m, s, s});
            if (pc_hud_ & 2) piece({kScreenW - kHud, 0, kHud, kHud}, {w - s - m, h - s - m, s, s});
            return;
        }
        const float s = 32.0f * k;
        if (pc_hud_ & 1) piece({0, 0, 32, 32}, {m, h - s - m, s, s});
        if (pc_hud_ & 2) piece({kScreenW - 32, 2, 32, 32}, {w - s - m, h - s - m, s, s});
    };
    const bool panel = pc_panel_top_ >= 0 && pc_panel_bottom_ > pc_panel_top_ && !text_top && pc_text_top_ < 0;
    if (panel) {
        const float top = std::clamp(std::floor(pc_panel_top_), 0.0f, float(kScreenH - 8));
        const float bottom = std::clamp(std::ceil(pc_panel_bottom_), top + 8, float(kScreenH));
        const float left = std::clamp(std::floor(pc_panel_left_), 0.0f, float(kScreenW - 8));
        const float right = std::clamp(std::ceil(pc_panel_right_), left + 8, float(kScreenW));
        const float tw = (right - left) * k, th = (bottom - top) * k;
        if (pc_hud_ && top >= kHud) hud_pieces();
        piece({left, top, right - left, bottom - top}, {std::round((w - tw) / 2), h - th - m, tw, th});
        return;
    }
    if (pc_hud_ && !text_top && !text_high) {
        // (the bag at the top left, the hand (talk) at the top right of the touch screen; only a corner the game
        // draws its HUD in now: while fishing it hides both, and a speech balloon reaching that rectangle would
        // otherwise show its piece in the window's corner and lose it where it is)
        hud_pieces();
    }
    const float tw = float(kScreenW * k);
    if (text_top) {
        // (a box the game put at the top of the touch screen: its rows from the top edge, at the top middle)
        const float bottom = std::clamp(std::ceil(pc_text_bottom_), 8.0f, float(kScreenH));
        const float th = bottom * k;
        piece({0, 0, float(kScreenW), bottom}, {std::round((w - tw) / 2), m, tw, th});
    } else if (pc_text_top_ >= 0) {
        const float top = std::clamp(std::floor(pc_text_top_), 0.0f, float(kScreenH - 8));
        const float th = (kScreenH - top) * k;
        piece({0, top, float(kScreenW), kScreenH - top}, {std::round((w - tw) / 2), h - th - m, tw, th});
    }
}

// PC view, a menu: the two screens side by side, as large as a whole-number scale allows, in the middle of the window
// over the dimmed picture from before it (only the touch screen takes clicks; the rest of the window none).
void Presenter::pc_modal_layout(int w, int h) {
    hole_count_ = 0;
    const int k = std::max(1, int(std::floor(std::min(w * 0.96f / (2 * kScreenW), h * 0.92f / kScreenH))));
    const float pw = float(kScreenW * k), ph = float(kScreenH * k), gap = float(4 * k);
    const float x = std::round((w - 2 * pw - gap) / 2), y = std::round((h - ph) / 2);
    view_count_ = 0;
    ScreenView back;
    back.screen = 2;
    back.rect = {0, 0, float(w), float(h)};
    back.picture = Picture::Backdrop;
    back.block = true;
    views_[view_count_++] = back;
    // (a box alone at the bottom of a black touch screen, the other screen black too: only the box's rows, where the
    // text box is in the world (bottom middle), at the text box's scale; the dimmed picture around it)
    if (pc_modal_show_ == 2 && pc_modal_band_ >= 0) {
        const int kb = std::max(1, std::min(pc_scale(h), w / kScreenW));
        const float top = std::clamp(std::floor(pc_modal_band_), 0.0f, float(kScreenH - 8));
        const float tw = float(kScreenW * kb), th = (kScreenH - top) * kb, m = std::round(h * 0.015f);
        ScreenView v;
        v.screen = 1;
        v.rect = {std::round((w - tw) / 2), h - th - m, tw, th};
        v.src = {0, top, float(kScreenW), kScreenH - top};
        v.shown = float(kScreenW);
        v.crisp = true;
        v.key_black = true;
        views_[view_count_++] = v;
        return;
    }
    // (one screen black: the other alone, in the middle, at the same scale)
    const bool one = pc_modal_show_ == 1 || pc_modal_show_ == 3;
    for (int screen = 0; screen < 2; screen++) {
        if (one && screen != (pc_modal_show_ == 1 ? 1 : 0)) continue;
        ScreenView v;
        v.screen = screen;
        v.rect = {one ? std::round((w - pw) / 2) : x + screen * (pw + gap), y, pw, ph};
        v.framed = true;
        v.shown = float(kScreenW);
        v.crisp = true;
        views_[view_count_++] = v;
    }
}

// PC view, both screens' 3D joined (views_[0] the top screen above views_[1] the bottom, laid out as One screen's): the
// two parts are the world alone, one camera's continuous view; the bottom screen's 2D is drawn over its own part (that
// is where it is touched); the top screen's 2D (the date, the time, the place) is a 4:3 panel in the top right corner,
// sized like One screen's corner picture, and while it covers the whole picture (a fade) over the top part as well.
void Presenter::pc_view_layout(int w, int h) {
    // (the ground and the sky: the bottom screen's whole picture below the lower part of the top screen's, pc_view_sky
    // of a screen (about 70% ground, 30% sky). The halves fill the window: the renderer's wide picture is a little
    // wider than the window's shape (its extra width comes in steps of 4, rounded up, see main.cpp wanted_video), and
    // the middle of it that fits is shown: no thin bars at the sides)
    // (looking up, set_look_up: the same height of the two screens' picture shown, moved up over it, to the top
    // screen whole above the upper part of the bottom one)
    const float tex_w = float(kScreenW + 2 * wide_extra_), ground = std::floor(h / (1 + pc_view_sky()));
    const float want = ground > 0 ? w * kScreenH / ground : 0;
    if (wide_extra_ > 0 && want >= float(kScreenW) && want <= tex_w + 0.5f) {
        const float row = ground / kScreenH;  // (window pixels per DS row)
        const float first = kScreenH * (1 - pc_view_sky()) * (1 - std::clamp(look_up_, 0.0f, 1.0f));  // (top screen rows above)
        const float sky = std::round((kScreenH - first) * row);
        views_[0].rect = {0, 0, float(w), sky};
        views_[0].crop_top = std::clamp(first / kScreenH, 0.0f, 0.9f);
        views_[1].rect = {0, sky, float(w), float(h) - sky};
        views_[1].crop_bottom = std::clamp(1 - (float(h) - sky) / row / kScreenH, 0.0f, 0.9f);
        views_[0].shown = views_[1].shown = std::min(want, tex_w);
    }
    views_[0].picture = views_[1].picture = Picture::World;
    const Rect top = views_[0].rect;
    int n = 2;
    views_[n] = views_[1];
    views_[n++].picture = Picture::Overlay;
    view_count_ = n;
    pc_parts(w, h, views_[1].rect);
    n = view_count_;
    // (the top screen's 2D over its own part too, as the touch screen's over its: what the game draws there sits over
    // the world it belongs to (the chanted words above the student); only the rows above that part, which the window
    // does not show, go to the corner panel. Testing, MQ_PCV_OLD_FILE: the whole 2D in the corner, over the world only
    // during a fade, as before)
    const bool top_2d = cover_top_ || !pc_old_;
    if (top_2d) {
        views_[n] = views_[0];
        views_[n++].picture = Picture::Overlay;
    }
    const float above = cover_top_ || pc_old_ ? float(kScreenH) : std::floor(views_[0].crop_top * kScreenH);
    int size = pip_;
    if (pip_attention_) size = pip_ == 2 ? 0 : 1;
    const float m = std::round(h * 0.02f);
    const float ph = std::min(std::round(h * (size == 1 ? 0.42f : 0.25f)), top.h - 2 * m), pw = std::round(ph * kScreenW / kScreenH);
    const float x = size == 2 ? float(w) + m : top.x + top.w - pw - m;
    views_[n++] = {0, {x, top.y + m, pw, ph}, 1.0f, false, float(kScreenW), 0.0f, 0.0f, Picture::Overlay};
    if (above < kScreenH) {
        if (above >= 8) {
            views_[n - 1].rect.h = std::round(ph * above / kScreenH);
            views_[n - 1].src = {0, 0, float(kScreenW), above};
        } else {
            n--;  // (all of it is over the world)
        }
    }
    view_count_ = n;
}

void Presenter::layout_views(int w, int h, const LayoutSettings& s) {
    width_ = w;
    height_ = h;
    pc_view_ = s.layout == Layout::PcView;
    hole_count_ = 0;
    if (pc_view_ && (pc_modal_ || flat_)) {
        pc_modal_layout(w, h);
        return;
    }
    // (PC view: the top screen is always above, the two halves being one camera's view)
    const int first = s.swap && s.layout != Layout::PcView ? 1 : 0, second = 1 - first;
    // Width over height of each screen: the top one is wider with widescreen.
    const float standard = float(kScreenW) / kScreenH;
    const bool flat = flat_ && joins_screens(s.layout);  // (2D screens: both 4:3, side by side)
    const float aspect[2] = {wide_extra_ && !flat ? float(kScreenW + 2 * wide_extra_) / kScreenH : standard,
                             wide_both_ && !flat ? float(kScreenW + 2 * wide_extra_) / kScreenH : standard};
    if (s.layout == Layout::Single || s.layout == Layout::Windows) {
        // The main screen as large as fits; the other in the bottom right corner, over the main one if it must.
        const float sh = std::min(float(h), w / aspect[first]), sw = sh * aspect[first];
        views_[0] = {first, {(w - sw) / 2, (h - sh) / 2, sw, sh}, 1.0f, false};
        view_count_ = 1;
        if (s.second_screen && s.layout == Layout::Single) {
            const float ph = std::min(h * std::clamp(s.pip_size, 0.1f, 0.5f), w / aspect[second] / 2);
            const float pw = ph * aspect[second], margin = std::round(h * 0.015f);
            views_[1] = {second, {w - pw - margin, h - ph - margin, pw, ph}, std::clamp(s.pip_opacity, 0.2f, 1.0f), true};
            view_count_ = 2;
        }
        return;
    }
    if (s.layout == Layout::MainSide) {
        // The main screen as tall as the window allows while leaving the side screen at least 30% of its width,
        // pinned to the left; the side screen takes the rest of the width, aligned to the main screen's top, middle
        // or bottom.
        const float mh = std::min(float(h), w / (aspect[first] * 1.3f));
        const Rect main{0, std::round((h - mh) / 2), mh * aspect[first], mh};
        float sw = w - main.w, sh = sw / aspect[second];
        if (sh > h) {
            sh = float(h);
            sw = sh * aspect[second];
        }
        const float sx = main.w + (w - main.w - sw) / 2;
        const float sy = s.side_align == Align::Top      ? main.y
                         : s.side_align == Align::Bottom ? main.y + main.h - sh
                                                         : (h - sh) / 2;
        views_[0] = {first, main, 1.0f, false};
        views_[1] = {second, {sx, std::clamp(sy, 0.0f, h - sh), sw, sh}, 1.0f, false};
        view_count_ = 2;
        return;
    }
    if (joins_screens(s.layout) && one_mode_ == OneMode::Solo) {
        solo_layout(w, h);
        return;
    }
    const bool stacked = s.layout == Layout::Stacked || (joins_screens(s.layout) && !flat);
    const float wa = aspect[first], wb = aspect[second];
    const float sh = stacked ? std::min(float(h) / 2, w / std::max(wa, wb)) : std::min(float(h), w / (wa + wb));
    const float wa_px = sh * wa, wb_px = sh * wb;
    if (stacked) {
        const float y = (h - sh * 2) / 2;
        views_[0] = {first, {(w - wa_px) / 2, y, wa_px, sh}, 1.0f, false};
        views_[1] = {second, {(w - wb_px) / 2, y + sh, wb_px, sh}, 1.0f, false};
    } else {
        const float x = (w - wa_px - wb_px) / 2, y = (h - sh) / 2;
        views_[0] = {first, {x, y, wa_px, sh}, 1.0f, false};
        views_[1] = {second, {x + wa_px, y, wb_px, sh}, 1.0f, false};
    }
    view_count_ = 2;
    if (joins_screens(s.layout))  // (the widths shown, for moving from one arrangement to another)
        for (int i = 0; i < 2; i++) views_[i].shown = shown_width(views_[i].screen);
    if (s.layout == Layout::PcView && !flat) pc_view_layout(w, h);
}

int Presenter::screen_at(float px, float py, float* local_x, float* local_y) const {
    world_hit_ = false;
    for (int i = view_count_ - 1; i >= 0; i--) {
        const Rect& r = views_[i].rect;
        if (!r.contains(px, py)) continue;
        if (views_[i].block) return -1;
        if (views_[i].screen > 1) continue;
        if (pc_view_ && views_[i].screen == 0) continue;  // (PC view: the top screen takes no clicks; those under it do)
        const Rect& src = views_[i].src;
        if (src.w > 0) {  // (a part drawn on a place of its own: where it is on the DS)
            *local_x = src.x + (px - r.x) / r.w * src.w;
            *local_y = src.y + (py - r.y) / r.h * src.h;
            return views_[i].screen;
        }
        // (a widened screen's DS pixels are its middle 256; a cropped one shows only some of its rows)
        const float sw = shown_width(views_[i]);
        const float ct = views_[i].crop_top, cb = views_[i].crop_bottom;
        *local_x = (px - r.x) / r.w * sw - (sw - kScreenW) / 2;
        *local_y = (ct + (py - r.y) / r.h * (1 - ct - cb)) * kScreenH;
        // (the parts drawn elsewhere take no clicks where they were)
        if (views_[i].screen == 1)
            for (int j = 0; j < hole_count_; j++)
                if (holes_[j].contains(*local_x, *local_y)) return -1;
        // (PC view: on the touch screen's world, where its 2D draws nothing (or beside the DS's rectangle): the game's
        // 3D picking's place, see pc_world_x)
        if (pc_view_ && views_[i].screen == 1 && !pc_modal_ && !flat_ && !pc_old_ && (wide_both_ && wide_extra_ > 0)) {
            const int gx = int(std::floor(*local_x / 4)), gy = int(std::floor(*local_y / 4));
            const bool inside = gx >= 0 && gx < 64 && gy >= 0 && gy < 48;
            world_hit_ = !inside || !bottom_alpha_ok_ || !bottom_alpha_[gy][gx];
        }
        return views_[i].screen;
    }
    return -1;
}

float Presenter::largest_screen_height() const {
    float h = 0;
    for (int i = 0; i < view_count_; i++) h = std::max(h, views_[i].rect.h);
    if (window2_) h = std::max(h, view2_.rect.h);
    return h;
}

// measure_screen: a 16 x 12 copy of the screen's DS pixels (a scaled blit), read back through a pixel buffer that is
// mapped only once the GPU is done with it (a fence), so nothing waits.
void Presenter::probe(const ntr::Frame* frame) {
    if (!probe_on_ || !frame || frame->top || !frame->gl_texture) return;
    // (the probe texture: the two screens' 16 x 12 copies side by side at its top left, and the PC view's top 2D part
    // at 128 x 96 beside them: small text must not fall between the points)
    // (and the touch screen's 2D part at 64 x 48 after them: how far up from its bottom edge it reaches, the text box)
    // (and the touch screen's whole picture at 64 x 48 last: a box alone on a black screen, bottom_lit_band)
    constexpr int kW = 32 + 128 + 64 + 64, kH = 96;
    if (!probe_tex_) {
        glGenFramebuffers(2, probe_fbo_);
        glGenTextures(1, &probe_tex_);
        glBindTexture(GL_TEXTURE_2D, probe_tex_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kW, kH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
        glGenBuffers(1, &probe_pbo_);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, probe_pbo_);
        glBufferData(GL_PIXEL_PACK_BUFFER, kW * kH * 4, nullptr, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }
    if (probe_fence_) {
        const GLenum r = glClientWaitSync(static_cast<GLsync>(probe_fence_), 0, 0);
        if (r != GL_ALREADY_SIGNALED && r != GL_CONDITION_SATISFIED) return;
        glDeleteSync(static_cast<GLsync>(probe_fence_));
        probe_fence_ = nullptr;
        glBindBuffer(GL_PIXEL_PACK_BUFFER, probe_pbo_);
        if (const auto* p = static_cast<const std::uint8_t*>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, kW * kH * 4, GL_MAP_READ_BIT))) {
            // (how many of the 192 points are lit: a loading screen's few sparkles leave a screen dark)
            int peak[2] = {0, 0};
            for (int y = 0; y < 12; y++)
                for (int x = 0; x < 32; x++) {
                    const std::uint8_t* q = p + (y * kW + x) * 4;
                    if (std::max({q[0], q[1], q[2]}) >= 14) peak[x / 16]++;
                }
            peak_[0] = peak[0];
            peak_[1] = peak[1];
            if (probe_overlay_) {
                int drawn = 0;
                for (int y = 0; y < 96; y++)
                    for (int x = 0; x < 128; x++) drawn += p[(y * kW + 32 + x) * 4 + 3] >= 24;
                peak_[2] = drawn;
            } else {
                peak_[2] = -1;
            }
            if (probe_bottom_) {
                int top = 48, wide_rows = 0;
                for (int y = 47; y >= 0; y--) {
                    int drawn = 0;
                    // (in the HUD's rows, DS 0-43, only what is between its corners counts: the bag and the hand
                    // alone do not carry a box's band up to the top)
                    const int x0 = y < 11 ? 10 : 0, x1 = y < 11 ? 54 : 64;
                    for (int x = x0; x < x1; x++) drawn += p[(y * kW + 160 + x) * 4 + 3] >= 24;
                    if (y >= 44 && drawn >= 40) wide_rows++;
                    if (!drawn) break;
                    top = y;
                }
                bottom_top_ = top * 4;
                bottom_box_ = wide_rows == 4;
                for (int y = 0; y < 48; y++)
                    for (int x = 0; x < 64; x++) bottom_alpha_[y][x] = p[(y * kW + 160 + x) * 4 + 3] >= 24;
                bottom_alpha_ok_ = true;
                // (the same from the top edge down: a text box the game put at the top of the touch screen)
                int bottom = -1, wide_top = 0;
                for (int y = 0; y < 48; y++) {
                    int drawn = 0;
                    for (int x = 0; x < 64; x++) drawn += p[(y * kW + 160 + x) * 4 + 3] >= 24;
                    if (y < 4 && drawn >= 40) wide_top++;
                    if (!drawn) break;
                    bottom = y;
                }
                bottom_bottom_ = (bottom + 1) * 4;
                bottom_top_box_ = wide_top == 4;
                int covered = 0;  // (rows drawn all across: a curtain over the whole screen)
                for (int y = 0; y < 48; y++) {
                    int drawn = 0;
                    for (int x = 0; x < 64; x++) drawn += p[(y * kW + 160 + x) * 4 + 3] >= 200;  // (opaque)
                    covered += drawn >= 60;
                }
                bottom_cover_ = covered >= 44;
                // (rows drawn opaque nearly all across at its bottom or top edge: a curtain coming in, a text box;
                // the modal's backdrop is not taken from such a picture)
                int edge_rows = 0;
                for (int y = 0; y < 48; y++) {
                    if (y >= 6 && y < 40) continue;
                    int drawn = 0;
                    for (int x = 0; x < 64; x++) drawn += p[(y * kW + 160 + x) * 4 + 3] >= 200;
                    edge_rows += drawn >= 48;
                }
                bottom_busy_ = edge_rows >= 3;
                // (a window: the longest run of rows drawn opaque across at least 24 of the 64 points, 96 DS pixels
                // (in the HUD's rows only between its corners), at least 8 rows (32 DS rows) tall: a menu alone; a
                // name tag, a thought balloon are smaller)
                int run = -1, best = -1, best_end = -1;
                for (int y = 0; y < 48; y++) {
                    int drawn = 0;
                    const int x0 = y < 11 ? 10 : 0, x1 = y < 11 ? 54 : 64;
                    for (int x = x0; x < x1; x++) drawn += p[(y * kW + 160 + x) * 4 + 3] >= 200;
                    if (drawn < 24) {
                        run = -1;
                        continue;
                    }
                    if (run < 0) run = y;
                    if (best < 0 || y + 1 - run > best_end - best) best = run, best_end = y + 1;
                }
                const bool panel = best >= 0 && best_end - best >= 8;
                bottom_panel_top_ = panel ? best * 4 : -1;
                bottom_panel_bottom_ = panel ? best_end * 4 : -1;
                // (... and its columns: the first and the one after the last drawn opaque in those rows)
                int left = 64, right = 0;
                for (int y = best; panel && y < best_end; y++) {
                    const int x0 = y < 11 ? 10 : 0, x1 = y < 11 ? 54 : 64;
                    for (int x = x0; x < x1; x++)
                        if (p[(y * kW + 160 + x) * 4 + 3] >= 200) left = std::min(left, x), right = std::max(right, x + 1);
                }
                bottom_panel_left_ = panel ? left * 4 : -1;
                bottom_panel_right_ = panel ? right * 4 : -1;
                if (static const bool log = std::getenv("MQ_LOG_PROBE") != nullptr; log) {
                    std::fprintf(stderr, "probe: bottom top %d wide rows %d covered %d rows top box %d to %d panel %d-%d", bottom_top_, wide_rows, covered,
                                 int(bottom_top_box_), bottom_bottom_, bottom_panel_top_, bottom_panel_bottom_);
                    std::fprintf(stderr, " cols %d-%d", bottom_panel_left_, bottom_panel_right_);
                    for (int y = 40; y < 48; y++) {
                        int drawn = 0;
                        for (int x = 0; x < 64; x++) drawn += p[(y * kW + 160 + x) * 4 + 3] >= 24;
                        std::fprintf(stderr, " %d", drawn);
                    }
                    std::fprintf(stderr, "\n");
                }
            } else {
                bottom_top_ = bottom_bottom_ = bottom_panel_top_ = bottom_panel_bottom_ = -1;
                bottom_box_ = bottom_cover_ = bottom_top_box_ = bottom_busy_ = bottom_alpha_ok_ = false;
            }
            {
                // (the lowest row from which every row down has a lit point, every row above none)
                int band = 48;
                auto lit = [&](int y) {
                    int n = 0;
                    for (int x = 0; x < 64; x++) {
                        const std::uint8_t* q = p + (y * kW + 224 + x) * 4;
                        n += std::max({q[0], q[1], q[2]}) >= 14;
                    }
                    return n;
                };
                for (int y = 47; y >= 0 && lit(y); y--) band = y;
                bool above = false;
                for (int y = 0; y < band && !above; y++) above = lit(y) > 0;
                bottom_band_ = pc_view_ && band < 48 && !above ? band * 4 : -1;
            }
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }
    if (++probe_count_ % 8 != 0) return;
    const int s = std::max(1, frame->scale), x0 = frame->wide_extra * s;
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, probe_fbo_[1]);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, probe_tex_, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, probe_fbo_[0]);
    for (int screen = 0; screen < 2; screen++) {
        glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, frame->gl_texture, 0, screen);
        glBlitFramebuffer(x0, 0, x0 + kScreenW * s, kScreenH * s, screen * 16, 0, screen * 16 + 16, 12,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    if (pc_view_) {
        glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, frame->gl_texture, 0, 1);
        glBlitFramebuffer(x0, 0, x0 + kScreenW * s, kScreenH * s, 224, 0, 224 + 64, 48, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    probe_overlay_ = frame->gl_overlay && frame->parts_fresh[0];
    if (probe_overlay_) {
        glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, frame->gl_overlay, 0, 0);
        glBlitFramebuffer(x0, 0, x0 + kScreenW * s, kScreenH * s, 32, 0, 32 + 128, 96, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    probe_bottom_ = frame->gl_overlay && frame->parts_fresh[1] && pc_view_;
    if (probe_bottom_) {
        glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, frame->gl_overlay, 0, 1);
        glBlitFramebuffer(x0, 0, x0 + kScreenW * s, kScreenH * s, 160, 0, 160 + 64, 48, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, probe_fbo_[1]);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, probe_pbo_);
    glReadPixels(0, 0, kW, kH, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    probe_fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Presenter::draw(const ntr::Frame* frame, Filter filter) {
    probe(frame);
#ifdef _WIN32
    target_ = flip_ ? flip_->begin(width_, height_) : 0;
#endif
    ScreenView shown[kMaxViews];
    const int count = current_views(shown);
    draw_views(target_, width_, height_, shown, count, frame, filter);
    // (the backdrop: a settled picture of the world, not one in a move between arrangements or with old parts, nor one
    // with a text box, a window or a curtain coming in on the touch screen: the world as it was before them)
    const bool moving = anim_start_ && anim_from_count_ && SDL_GetTicks() - anim_start_ < 300;
    if (pc_view_ && frame && !frame->top && frame->parts_fresh[1] && frame->wide_3d && !moving && !(pc_modal_ || flat_) &&
        !bottom_busy_ && pc_text_top_ < 0 && pc_text_bottom_ < 0 && pc_panel_top_ < 0)
        save_backdrop();
}

// PC view: the picture just drawn kept (upside down into a layer of an array, as the screens' shader reads), the
// backdrop of a modal that opens next.
void Presenter::save_backdrop() {
    if (width_ <= 0 || height_ <= 0) return;
    if (!backdrop_tex_ || backdrop_w_ != width_ || backdrop_h_ != height_) {
        if (!backdrop_tex_) {
            GLuint t, f;
            glGenTextures(1, &t);
            glGenFramebuffers(1, &f);
            backdrop_tex_ = t;
            backdrop_fbo_ = f;
        }
        glBindTexture(GL_TEXTURE_2D_ARRAY, backdrop_tex_);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, width_, height_, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        backdrop_layer_ = backdrop_count_ = backdrop_full_ = 0;
        backdrop_ok_ = false;
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        backdrop_w_ = width_;
        backdrop_h_ = height_;
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, backdrop_fbo_);
    if (++backdrop_count_ > 10) {
        backdrop_count_ = 1;
        backdrop_layer_ = (backdrop_layer_ + 1) % 3;
        backdrop_full_++;
    }
    glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, backdrop_tex_, 0, backdrop_layer_);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, target_);
    if (!target_) glReadBuffer(GL_BACK);
    glBlitFramebuffer(0, 0, width_, height_, 0, height_, width_, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, target_);
    backdrop_ok_ = backdrop_full_ >= 2;
    backdrop_area_ = pc_area_;
}

void Presenter::draw_views(unsigned target, int width, int height, const ScreenView* views, int count,
                           const ntr::Frame* frame, Filter filter) {
    // melonDS's GL renderers share this context: put back the state the screen pass relies on.
    glBindFramebuffer(GL_FRAMEBUFFER, target);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glViewport(0, 0, width, height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    if (frame) {
        glActiveTexture(GL_TEXTURE0);
        if (frame->top) {
            glBindTexture(GL_TEXTURE_2D_ARRAY, soft_tex_);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, kScreenW, kScreenH, 1, GL_BGRA, GL_UNSIGNED_BYTE, frame->top);
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, kScreenW, kScreenH, 1, GL_BGRA, GL_UNSIGNED_BYTE,
                            frame->bottom);
        }
        const GLint f = filter == Filter::Nearest ? GL_NEAREST : GL_LINEAR;

        glUseProgram(program_);
        glUniform1i(u_tex_, 0);
        glUniform1i(u_sharp_, filter == Filter::Sharp);
        glBindVertexArray(vao_);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for (int i = 0; i < count; i++) {
            const ScreenView& v = views[i];
            const Rect& r = v.rect;
            if (v.picture == Picture::Backdrop) {  // (the last picture before a modal, darkened)
                // (kept in another area (a scene with 3D on the top screen only reached straight from the town, the
                // menu not one of the field's own (the bag, the Start menu)): the top screen's live picture instead,
                // filling the window (its middle), darkened more)
                const bool stale = !backdrop_ok_ || (pc_area_ >= 0 && backdrop_area_ >= 0 && pc_area_ != backdrop_area_);
                if (stale && pc_top_live_ && flat_ && !pc_modal_ && !pc_old_ && !frame->top && frame->gl_texture) {
                    glActiveTexture(GL_TEXTURE0);
                    // (its world alone where the renderer split it: the 2D over it, the words' slots, stays in its pane)
                    glBindTexture(GL_TEXTURE_2D_ARRAY,
                                  frame->parts_fresh[0] && frame->gl_world ? frame->gl_world : frame->gl_texture);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glUniform1i(u_sharp_, 0);
                    glUniform1i(u_alpha_, 0);
                    glUniform1i(u_hole_count_, 0);
                    const float tw = float(kScreenW + 2 * frame->wide_extra), src = tw / kScreenH, dst = r.w / std::max(1.0f, r.h);
                    const float ux = dst < src ? 0.5f * (1 - dst / src) : 0.0f, uy = dst > src ? 0.5f * (1 - src / dst) : 0.0f;
                    glUniform2f(u_uv_x_, ux, 1 - ux);
                    glUniform2f(u_uv_y_, uy, 1 - uy);
                    glUniform4f(u_dest_, r.x / width * 2 - 1, 1 - r.y / height * 2, (r.x + r.w) / width * 2 - 1,
                                1 - (r.y + r.h) / height * 2);
                    glUniform1f(u_layer_, 0.0f);
                    glUniform2f(u_out_size_, r.w, r.h);
                    glUniform1f(u_opacity_, 1.0f);
                    glEnable(GL_BLEND);
                    glBlendColor(0.3f, 0.3f, 0.3f, 1.0f);
                    glBlendFunc(GL_CONSTANT_COLOR, GL_ZERO);  // (the picture times 0.3)
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    glBlendColor(0, 0, 0, 0);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glDisable(GL_BLEND);
                    continue;
                }
                if (!backdrop_ok_ || !backdrop_tex_) continue;
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D_ARRAY, backdrop_tex_);
                glUniform1i(u_alpha_, 0);
                glUniform1i(u_hole_count_, 0);
                glUniform2f(u_uv_x_, 0.0f, 1.0f);
                glUniform2f(u_uv_y_, 0.0f, 1.0f);
                glUniform4f(u_dest_, r.x / width * 2 - 1, 1 - r.y / height * 2, (r.x + r.w) / width * 2 - 1,
                            1 - (r.y + r.h) / height * 2);
                glUniform1f(u_layer_, float((backdrop_layer_ + 1) % 3));
                glUniform2f(u_out_size_, r.w, r.h);
                glUniform1f(u_opacity_, 1.0f);
                glEnable(GL_BLEND);
                glBlendColor(0.4f, 0.4f, 0.4f, 1.0f);
                glBlendFunc(GL_CONSTANT_COLOR, GL_ZERO);  // (the picture times 0.4)
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBlendColor(0, 0, 0, 0);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable(GL_BLEND);
                continue;
            }
            // (the PC view's parts: the world falls back to the whole picture, and the 2D is left out, where the
            // renderer makes no parts or the screen's are old: it has had no 3D since, a shop's 2D catalogue)
            const bool overlay = v.picture == Picture::Overlay;
            const bool parts = !frame->top && frame->parts_fresh[v.screen];
            unsigned tex = frame->top ? soft_tex_ : frame->gl_texture;
            if (v.picture == Picture::World && parts && frame->gl_world) tex = frame->gl_world;
            if (overlay) tex = parts ? frame->gl_overlay : 0;
            if (!tex) continue;
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, v.crisp ? GL_NEAREST : f);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, v.crisp ? GL_NEAREST : f);
            glUniform1i(u_sharp_, filter == Filter::Sharp && !v.crisp);
            // (PC view: the touch screen's parts drawn elsewhere are left out of its whole 2D)
            const float full_w = frame->top ? float(kScreenW) : float(kScreenW + 2 * frame->wide_extra);
            const float ds_u0 = (full_w - kScreenW) / 2 / full_w;
            int holes = 0;
            if (overlay && v.screen == 1 && v.src.w <= 0 && pc_view_) {
                float hv[16];
                // (a pixel wider each way, so filtering leaves no edge of them behind; to the texture's edges where they
                // reach the DS screen's)
                for (int j = 0; j < hole_count_ && j < 4; j++, holes++) {
                    const Rect& hr = holes_[j];
                    hv[4 * j + 0] = hr.x <= 0 ? -1.0f : ds_u0 + (hr.x - 1) / full_w;
                    hv[4 * j + 1] = hr.y <= 0 ? -1.0f : (hr.y - 1) / kScreenH;
                    hv[4 * j + 2] = hr.x + hr.w >= kScreenW ? 2.0f : ds_u0 + (hr.x + hr.w + 1) / full_w;
                    hv[4 * j + 3] = hr.y + hr.h >= kScreenH ? 2.0f : (hr.y + hr.h + 1) / kScreenH;
                }
                if (holes) glUniform4fv(u_holes_, holes, hv);
            }
            glUniform1i(u_hole_count_, holes);
            if (v.src.w > 0) {  // (a part of the screen on a place of its own)
                if (v.opacity < 1.0f || overlay) glEnable(GL_BLEND);
                glBlendFunc(overlay ? GL_ONE : GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glUniform1i(u_alpha_, v.key_black ? 2 : overlay);
                const float u0 = ds_u0 + v.src.x / full_w, u1 = ds_u0 + (v.src.x + v.src.w) / full_w;
                const float v0 = v.src.y / kScreenH, v1 = (v.src.y + v.src.h) / kScreenH;
                glUniform2f(u_uv_x_, u0, u1);
                glUniform2f(u_uv_y_, v0, v1);
                glUniform4f(u_dest_, r.x / width * 2 - 1, 1 - r.y / height * 2, (r.x + r.w) / width * 2 - 1,
                            1 - (r.y + r.h) / height * 2);
                glUniform1f(u_layer_, float(v.screen));
                glUniform2f(u_out_size_, r.w / std::max(1e-6f, u1 - u0), r.h / std::max(1e-6f, v1 - v0));
                glUniform1f(u_opacity_, v.opacity);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glDisable(GL_BLEND);
                continue;
            }
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glUniform1i(u_alpha_, overlay);
            const float crop = std::clamp(v.crop_top, 0.0f, 0.9f);  // (PC view: the sky part shows the top screen's lower part)
            const float crop_b = std::clamp(v.crop_bottom, 0.0f, 0.9f - crop);  // (... and looking up, the ground its upper)
            glUniform2f(u_uv_y_, crop, 1.0f - crop_b);
            if (v.framed && v.opacity >= 1.0f) {
                // A thin border sets the picture-in-picture apart from the screen under it.
                const int b = std::max(2, height / 400);
                glEnable(GL_SCISSOR_TEST);
                glScissor(int(r.x) - b, height - int(r.y + r.h) - b, int(r.w) + 2 * b, int(r.h) + 2 * b);
                glClearColor(0.55f, 0.6f, 0.7f, 1);
                glClear(GL_COLOR_BUFFER_BIT);
                if (overlay) {  // (a panel of 2D alone: on a dark backing)
                    glScissor(int(r.x), height - int(r.y + r.h), int(r.w), int(r.h));
                    glClearColor(0.06f, 0.06f, 0.08f, 1);
                    glClear(GL_COLOR_BUFFER_BIT);
                }
                glDisable(GL_SCISSOR_TEST);
            }
            if (v.opacity < 1.0f || overlay) glEnable(GL_BLEND);
            glBlendFunc(overlay ? GL_ONE : GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // The renderer's output is wider with widescreen: the bottom screen is its middle part.
            // (flat: both screens are the middle part)
            // (a view with a width of its own shows that much of the middle)
            const float tex_w = frame->top ? float(kScreenW) : float(kScreenW + 2 * frame->wide_extra);
            float sw = v.shown > 0 ? v.shown : tex_w;
            if (v.shown <= 0 && frame->wide_extra > 0 && !frame->top && (flat_ || (v.screen == 1 && !frame->wide_both)))
                sw = kScreenW;
            // One screen, joined, a screen without 3D (a shop's 2D catalogue under its 3D top screen): its 4:3 picture in
            // the middle of its wide place, over a dim, stretched copy of itself (no black bars)
            Rect pr = r;
            // (PC view: a screen whose parts are old (no 3D on it now: the bag's top screen) shows only its DS pixels:
            // its wide sides would be left over from the last frame with 3D)
            if (v.picture == Picture::World && !parts && !frame->top && frame->wide_extra > 0) {
                sw = kScreenW;
                const float cw = std::min(r.w, r.h / (1 - crop - crop_b) * kScreenW / kScreenH);
                pr = {r.x + (r.w - cw) / 2, r.y, cw, r.h};
            } else if (ambient_[v.screen] && !frame->top && !overlay) {
                const float in = std::min(1.0f, float(kScreenW) / tex_w), a0 = (1 - in) / 2;
                glEnable(GL_BLEND);
                glUniform2f(u_uv_x_, a0, 1 - a0);
                glUniform4f(u_dest_, r.x / width * 2 - 1, 1 - r.y / height * 2, (r.x + r.w) / width * 2 - 1,
                            1 - (r.y + r.h) / height * 2);
                glUniform1f(u_layer_, float(v.screen));
                glUniform2f(u_out_size_, r.w, r.h);
                glUniform1f(u_opacity_, 0.3f * v.opacity);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                if (v.opacity >= 1.0f) glDisable(GL_BLEND);
                sw = kScreenW;
                const float cw = std::min(r.w, r.h * kScreenW / kScreenH);
                pr = {r.x + (r.w - cw) / 2, r.y, cw, r.h};
            }
            const float inner = std::min(1.0f, sw / tex_w);
            const float u0 = (1 - inner) / 2, u1 = 1 - u0;
            glUniform2f(u_uv_x_, u0, u1);
            glUniform4f(u_dest_, pr.x / width * 2 - 1, 1 - pr.y / height * 2, (pr.x + pr.w) / width * 2 - 1,
                        1 - (pr.y + pr.h) / height * 2);
            glUniform1f(u_layer_, float(v.screen));
            glUniform2f(u_out_size_, inner < 1.0f ? pr.w / inner : pr.w, pr.h / (1 - crop - crop_b));
            glUniform1f(u_opacity_, v.opacity);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            // (PC view, a fade: the wide sides darkened as the game's black layer darkens the DS pixels between them)
            if (v.picture == Picture::World && pc_view_ && fade_dark_ > 0 && parts && sw > kScreenW) {
                const float mid = pr.w * kScreenW / sw, side = (pr.w - mid) / 2, k = 1 - std::clamp(fade_dark_, 0.0f, 1.0f);
                glEnable(GL_BLEND);
                glBlendFunc(GL_ZERO, GL_CONSTANT_COLOR);
                glBlendColor(k, k, k, k);
                for (const float x0 : {pr.x, pr.x + pr.w - side}) {
                    glUniform4f(u_dest_, x0 / width * 2 - 1, 1 - pr.y / height * 2, (x0 + side) / width * 2 - 1,
                                1 - (pr.y + pr.h) / height * 2);
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                }
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glBlendColor(0, 0, 0, 0);
            }
            if (picture_on_ && picture_tex_ && v.screen == 0 && !overlay) draw_picture(r, v.opacity, filter, width, height, frame);
            glDisable(GL_BLEND);
        }
        glUniform1i(u_alpha_, 0);
        glUniform1i(u_hole_count_, 0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(0);
    }
    if (cursor_on_)
        for (int i = 0; i < count; i++)
            if (views[i].screen == 1 && views[i].picture != Picture::Overlay && views[i].src.w <= 0)
                draw_cursor(views[i], height);
}

// A crosshair of scissored clears (no shader): white on a dark outline, with a yellow middle while touching.
void Presenter::draw_cursor(const ScreenView& v, int height) {
    const Rect& r = v.rect;
    const float sw = shown_width(v), unit = r.h / kScreenH;
    const float rows = kScreenH * std::max(0.1f, 1 - v.crop_top - v.crop_bottom);
    const float cx = r.x + (cursor_x_ + 0.5f + (sw - kScreenW) / 2) / sw * r.w,
                cy = r.y + (cursor_y_ + 0.5f - v.crop_top * kScreenH) / rows * r.h;
    const float arm = 9 * unit, half = std::max(1.0f, 0.75f * unit), edge = std::max(1.0f, 0.6f * unit);
    auto box = [&](float x0, float y0, float x1, float y1, float red, float green, float blue) {
        const int ix = int(std::floor(x0)), iy = int(std::floor(y0));
        const int iw = std::max(1, int(std::ceil(x1)) - ix), ih = std::max(1, int(std::ceil(y1)) - iy);
        glScissor(ix, height - iy - ih, iw, ih);
        glClearColor(red, green, blue, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    };
    glEnable(GL_SCISSOR_TEST);
    box(cx - arm - edge, cy - half - edge, cx + arm + edge, cy + half + edge, 0.1f, 0.1f, 0.15f);
    box(cx - half - edge, cy - arm - edge, cx + half + edge, cy + arm + edge, 0.1f, 0.1f, 0.15f);
    box(cx - arm, cy - half, cx + arm, cy + half, 1, 1, 1);
    box(cx - half, cy - arm, cx + half, cy + arm, 1, 1, 1);
    if (cursor_pressed_) box(cx - 2.5f * half, cy - 2.5f * half, cx + 2.5f * half, cy + 2.5f * half, 1.0f, 0.8f, 0.2f);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 0, 1);
}

// The set_picture picture over the part of the top screen (drawn into `r`) that it covers.
void Presenter::draw_picture(const Rect& r, float opacity, Filter filter, int width, int height, const ntr::Frame* frame) {
    // The widescreen renderer's output is wider than the DS screen: its middle part is the 256 pixels the game draws.
    const bool wide = frame->wide_extra > 0 && !frame->top;
    const float inner = wide ? float(kScreenW) / (kScreenW + 2 * frame->wide_extra) : 1.0f;
    const float fx0 = (1 - inner) / 2 + inner * picture_ds_.x / kScreenW, fx1 = (1 - inner) / 2 + inner * (picture_ds_.x + picture_ds_.w) / kScreenW;
    const float fy0 = picture_ds_.y / kScreenH, fy1 = (picture_ds_.y + picture_ds_.h) / kScreenH;
    const Rect d{r.x + r.w * fx0, r.y + r.h * fy0, r.w * (fx1 - fx0), r.h * (fy1 - fy0)};
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, picture_tex_);
    const GLint f = filter == Filter::Nearest ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, f);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, f);
    glUniform2f(u_uv_x_, 0.0f, 1.0f);
    glUniform2f(u_uv_y_, 0.0f, 1.0f);
    glUniform4f(u_dest_, d.x / width * 2 - 1, 1 - d.y / height * 2, (d.x + d.w) / width * 2 - 1, 1 - (d.y + d.h) / height * 2);
    glUniform1f(u_layer_, 0.0f);
    glUniform2f(u_out_size_, d.w, d.h);
    glUniform1f(u_opacity_, opacity);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindTexture(GL_TEXTURE_2D_ARRAY, frame->top ? soft_tex_ : frame->gl_texture);  // (the screens' texture, for the next view)
}

bool Presenter::open_second(SDL_Window* window, bool allow_flip_model) {
    close_second();
    window2_ = window;
#ifdef _WIN32
    if (allow_flip_model && flip_) {  // (the first window's choice: a GL swap if it had none)
        void* hwnd = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        auto flip = std::make_unique<FlipPresenter>();
        std::string error;
        if (hwnd && flip->init(hwnd, error))
            flip2_ = std::move(flip);
        else
            std::printf("second window: OpenGL swap (flip model unavailable: %s)\n", error.c_str());
    }
#else
    (void)allow_flip_model;
#endif
    return true;
}

void Presenter::close_second() {
    flip2_.reset();
    window2_ = nullptr;
}

void Presenter::layout_second(int w, int h, const LayoutSettings& s) {
    width2_ = w;
    height2_ = h;
    const int second = s.swap ? 0 : 1;  // the screen that is not the main window's
    const float aspect = (second == 0 || wide_both_) && wide_extra_ ? float(kScreenW + 2 * wide_extra_) / kScreenH
                                                    : float(kScreenW) / kScreenH;
    const float sh = std::min(float(h), w / aspect), sw = sh * aspect;
    view2_ = {second, {(w - sw) / 2, (h - sh) / 2, sw, sh}, 1.0f, false};
}

int Presenter::screen_at_second(float px, float py, float* local_x, float* local_y) const {
    if (!window2_ || !view2_.rect.contains(px, py)) return -1;
    const float sw = shown_width(view2_.screen);
    *local_x = (px - view2_.rect.x) / view2_.rect.w * sw - (sw - kScreenW) / 2;
    *local_y = (py - view2_.rect.y) / view2_.rect.h * kScreenH;
    return view2_.screen;
}

void Presenter::show_second(const ntr::Frame* frame, Filter filter, const char* screenshot) {
    unsigned target = 0;
#ifdef _WIN32
    if (flip2_) target = flip2_->begin(width2_, height2_);
#endif
    if (!flip2_) {
        SDL_GL_MakeCurrent(window2_, static_cast<SDL_GLContext>(context_));
        SDL_GL_SetSwapInterval(0);
    }
    draw_views(target, width2_, height2_, &view2_, 1, frame, filter);
    if (screenshot) {  // (named like the first window's, with -window2 before the extension)
        std::string path = screenshot;
        const size_t dot = path.rfind('.');
        path.insert(dot == std::string::npos ? path.size() : dot, "-window2");
        save_png(path.c_str(), target, width2_, height2_);
    }
#ifdef _WIN32
    if (flip2_) {
        flip2_->present(false);  // (at once: the first window's vsync is the one that paces the frames)
        return;
    }
#endif
    SDL_GL_SwapWindow(window2_);
    SDL_GL_MakeCurrent(window_, static_cast<SDL_GLContext>(context_));
    SDL_GL_SetSwapInterval(flip_ || !vsync_ ? 0 : 1);
}

void Presenter::swap(const char* screenshot) {
    if (screenshot) save_png(screenshot, target_, width_, height_);
    if (hash_pictures_) {
        if (SDL_Surface* surf = read_back(target_, width_, height_)) {
            picture_hash_ = ntr::hash64(surf->pixels, size_t(surf->pitch) * size_t(surf->h), picture_hash_);
            SDL_DestroySurface(surf);
        }
    }
#ifdef _WIN32
    if (flip_) {
        flip_->present(vsync_, tearing_);
        return;
    }
#endif
    SDL_GL_SwapWindow(window_);
}

void Presenter::set_vsync(bool on) {
    if (on == vsync_) return;
    vsync_ = on;
    if (!flip_) SDL_GL_SetSwapInterval(on ? 1 : 0);  // (a GL swap with vsync waits for the refresh itself)
}

void Presenter::wait_for_refresh() {
#ifdef _WIN32
    if (flip_ && vsync_) flip_->wait();
#endif
}

bool Presenter::tearing_supported() const {
#ifdef _WIN32
    if (flip_) return flip_->tearing_supported();
#endif
    return true;  // (a GL swap without vsync tears by itself)
}

float Presenter::refresh_rate() const {
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window_));
    return mode ? mode->refresh_rate : 0.0f;
}

}  // namespace video
