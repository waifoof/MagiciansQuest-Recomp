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
uniform float uLayer;
in vec2 vPos;        // unit quad, (0,0) = top left
out vec3 fTex;
void main() {
    gl_Position = vec4(mix(uDest.xy, uDest.zw, vPos), 0.0, 1.0);
    fTex = vec3(mix(uUvX.x, uUvX.y, vPos.x), vPos.y, uLayer);
}
)";

// Sharp bilinear (after Themaister's shader): magnify each texel by the largest integer factor that fits and blend
// only across the remaining fraction of an output pixel at texel edges. Needs the texture's filter set to linear.
const char* kFS = R"(#version 140
uniform sampler2DArray uTex;
uniform int uSharp;
uniform vec2 uOutSize;  // the screen's size on the window, in pixels
uniform float uOpacity;
in vec3 fTex;
out vec4 oColor;
void main() {
    vec2 uv = fTex.xy;
    if (uSharp != 0) {
        vec2 size = vec2(textureSize(uTex, 0).xy);
        vec2 texel = uv * size;
        vec2 scale = max(floor(uOutSize / size), vec2(1.0));
        vec2 d = fract(texel) - 0.5;
        vec2 range = 0.5 - 0.5 / scale;
        uv = (floor(texel) + (d - clamp(d, -range, range)) * scale + 0.5) / size;
    }
    oColor = vec4(texture(uTex, vec3(uv, fTex.z)).rgb, uOpacity);
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
    ScreenView before[2];
    const int before_count = current_views(before);
    const bool one = s.layout == Layout::OneScreen;
    const bool moved = one && before_count == 2 && laid_one_ &&
                       (one_mode_ != laid_mode_ || pip_ != laid_pip_ || pip_attention_ != laid_attention_ ||
                        solo_screen_ != laid_solo_);
    laid_one_ = one;
    laid_mode_ = one_mode_;
    laid_pip_ = pip_;
    laid_attention_ = pip_attention_;
    laid_solo_ = solo_screen_;
    layout_views(w, h, s);
    if (moved) {
        for (int i = 0; i < 2; i++) anim_from_[i] = before[i];
        anim_from_count_ = 2;
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
        const ScreenView* from = nullptr;
        for (int j = 0; j < anim_from_count_; j++)
            if (anim_from_[j].screen == views_[i].screen) from = &anim_from_[j];
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
    const float x = size == 2 ? float(w) + m : r.x + r.w - pw - m;
    views_[1] = {other, {x, r.y + m, pw, ph}, 1.0f, true, float(kScreenW)};
    view_count_ = 2;
}

void Presenter::layout_views(int w, int h, const LayoutSettings& s) {
    width_ = w;
    height_ = h;
    const int first = s.swap ? 1 : 0, second = 1 - first;
    // Width over height of each screen: the top one is wider with widescreen.
    const float standard = float(kScreenW) / kScreenH;
    const bool flat = flat_ && s.layout == Layout::OneScreen;  // (2D screens: both 4:3, side by side)
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
    if (s.layout == Layout::OneScreen && one_mode_ == OneMode::Solo) {
        solo_layout(w, h);
        return;
    }
    const bool stacked = s.layout == Layout::Stacked || (s.layout == Layout::OneScreen && !flat);
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
    if (s.layout == Layout::OneScreen)  // (the widths shown, for moving from one arrangement to another)
        for (int i = 0; i < 2; i++) views_[i].shown = shown_width(views_[i].screen);
}

int Presenter::screen_at(float px, float py, float* local_x, float* local_y) const {
    for (int i = view_count_ - 1; i >= 0; i--) {
        const Rect& r = views_[i].rect;
        if (!r.contains(px, py)) continue;
        // (a widened screen's DS pixels are its middle 256)
        const float sw = shown_width(views_[i]);
        *local_x = (px - r.x) / r.w * sw - (sw - kScreenW) / 2;
        *local_y = (py - r.y) / r.h * kScreenH;
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
    if (!probe_tex_) {
        glGenFramebuffers(2, probe_fbo_);
        glGenTextures(1, &probe_tex_);
        glBindTexture(GL_TEXTURE_2D, probe_tex_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 32, 12, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
        glGenBuffers(1, &probe_pbo_);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, probe_pbo_);
        glBufferData(GL_PIXEL_PACK_BUFFER, 32 * 12 * 4, nullptr, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }
    if (probe_fence_) {
        const GLenum r = glClientWaitSync(static_cast<GLsync>(probe_fence_), 0, 0);
        if (r != GL_ALREADY_SIGNALED && r != GL_CONDITION_SATISFIED) return;
        glDeleteSync(static_cast<GLsync>(probe_fence_));
        probe_fence_ = nullptr;
        glBindBuffer(GL_PIXEL_PACK_BUFFER, probe_pbo_);
        if (const auto* p = static_cast<const std::uint8_t*>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, 32 * 12 * 4, GL_MAP_READ_BIT))) {
            // (how many of the 192 points are lit: a loading screen's few sparkles leave a screen dark)
            int peak[2] = {0, 0};
            for (int y = 0; y < 12; y++)
                for (int x = 0; x < 32; x++) {
                    const std::uint8_t* q = p + (y * 32 + x) * 4;
                    if (std::max({q[0], q[1], q[2]}) >= 14) peak[x / 16]++;
                }
            peak_[0] = peak[0];
            peak_[1] = peak[1];
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
    glBindFramebuffer(GL_READ_FRAMEBUFFER, probe_fbo_[1]);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, probe_pbo_);
    glReadPixels(0, 0, 32, 12, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
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
    ScreenView shown[2];
    const int count = current_views(shown);
    draw_views(target_, width_, height_, shown, count, frame, filter);
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
        } else {
            glBindTexture(GL_TEXTURE_2D_ARRAY, frame->gl_texture);
        }
        const GLint f = filter == Filter::Nearest ? GL_NEAREST : GL_LINEAR;
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, f);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, f);

        glUseProgram(program_);
        glUniform1i(u_tex_, 0);
        glUniform1i(u_sharp_, filter == Filter::Sharp);
        glBindVertexArray(vao_);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for (int i = 0; i < count; i++) {
            const ScreenView& v = views[i];
            const Rect& r = v.rect;
            if (v.framed && v.opacity >= 1.0f) {
                // A thin border sets the picture-in-picture apart from the screen under it.
                const int b = std::max(2, height / 400);
                glEnable(GL_SCISSOR_TEST);
                glScissor(int(r.x) - b, height - int(r.y + r.h) - b, int(r.w) + 2 * b, int(r.h) + 2 * b);
                glClearColor(0.55f, 0.6f, 0.7f, 1);
                glClear(GL_COLOR_BUFFER_BIT);
                glDisable(GL_SCISSOR_TEST);
            }
            if (v.opacity < 1.0f) glEnable(GL_BLEND);
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
            if (ambient_[v.screen] && !frame->top) {
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
            glUniform2f(u_out_size_, inner < 1.0f ? pr.w / inner : pr.w, pr.h);
            glUniform1f(u_opacity_, v.opacity);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            if (picture_on_ && picture_tex_ && v.screen == 0) draw_picture(r, v.opacity, filter, width, height, frame);
            glDisable(GL_BLEND);
        }
        glUseProgram(0);
    }
    if (cursor_on_)
        for (int i = 0; i < count; i++)
            if (views[i].screen == 1) draw_cursor(views[i], height);
}

// A crosshair of scissored clears (no shader): white on a dark outline, with a yellow middle while touching.
void Presenter::draw_cursor(const ScreenView& v, int height) {
    const Rect& r = v.rect;
    const float sw = shown_width(v), unit = r.h / kScreenH;
    const float cx = r.x + (cursor_x_ + 0.5f + (sw - kScreenW) / 2) / sw * r.w, cy = r.y + (cursor_y_ + 0.5f) / kScreenH * r.h;
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
