// OpenGL presentation of the DS screens: owns the window's GL context (which melonDS's GL renderers also draw with)
// and scales the two screens into the window.
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <functional>
#include <memory>
#include <vector>

#include "ntr/runtime.hpp"

struct SDL_Surface;
struct SDL_Window;

namespace video {

// How the screen images are filtered when scaled to the window.
enum class Filter {
    Nearest,  // hard pixel edges, uneven at non-integer sizes
    Linear,   // smooth (soft at large magnification)
    Sharp,    // sharp bilinear: integer-scaled pixels with a one-pixel blend at the edges
};
const char* filter_name(Filter f);

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;  // in window pixels, origin at the top left
    bool contains(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

// Where the two screens go on the window.
enum class Layout {
    Single,      // one screen fills the window; the other is an optional picture-in-picture
    MainSide,    // the main screen at the left edge; the other fills the width that is left, on the right
    Stacked,     // one above the other, as on the console
    SideBySide,  // next to each other, the same size
    Windows,     // the main screen fills the window; the other one fills a second window
    OneScreen,   // one picture: stacked with no gap, both widened to fill the window (a game with 3D on both screens)
    PcView,      // one picture of the world from one camera (docs/PC_VIEW.md): as One screen, with the 3D alone, the
                 // bottom screen's 2D over its part and the top screen's 2D in a corner panel
};
const char* layout_name(Layout l);
// The layouts that join the two screens into one picture, arranged by what the game shows (see OneMode).
inline bool joins_screens(Layout l) { return l == Layout::OneScreen || l == Layout::PcView; }
// PC view, joined: how much of the top screen's picture is shown above the bottom one's, as a part of a screen's height
// (0.43: about 70% ground, 30% sky; 1: the two screens whole; MQ_PCV_SKY to try others).
inline float pc_view_sky() {
    static const float sky = [] {
        const char* e = std::getenv("MQ_PCV_SKY");
        return e && *e ? std::clamp(float(std::atof(e)), 0.1f, 1.0f) : 0.43f;
    }();
    return sky;
}

// Where the smaller screen sits vertically in the MainSide layout.
enum class Align { Top, Middle, Bottom };
const char* align_name(Align a);

struct LayoutSettings {
    Layout layout = Layout::SideBySide;  // Magician's Quest plays on both screens (the character often walks on the bottom one)
    bool swap = false;           // Single/MainSide: the bottom screen is the main one. Others: the bottom screen first.
    bool second_screen = true;   // Single: show the other screen as a picture-in-picture
    float pip_size = 0.3f;       // picture-in-picture height, as a fraction of the window height
    float pip_opacity = 1.0f;
    Align side_align = Align::Bottom;  // MainSide: the side screen's vertical position
    bool one_screen_solo = true;  // One screen: where the game has 3D on the bottom screen only, that one fills the window
    int one_pip = 0;              // One screen: the other screen's picture-in-picture: 0 small, 1 large, 2 hidden
};

// What of a screen a view shows: all of it, or (PC view) one of its two parts (ntr::Frame::gl_world, gl_overlay).
enum class Picture {
    Composite,  // the screen as the DS shows it
    World,      // the 3D and what is behind it (the whole screen where there are no parts: the software renderer)
    Overlay,    // the 2D in front of the 3D, see-through elsewhere (not drawn where there are no parts)
    Backdrop,   // (PC view, a menu's modal) the window's last picture before the menu opened, dimmed
};

// One DS screen's place on the window.
struct ScreenView {
    int screen = 0;  // 0 = top, 1 = bottom
    Rect rect;
    float opacity = 1;
    bool framed = false;  // drawn with a border (the picture-in-picture)
    float shown = 0;      // the width shown, in DS pixels about the screen's middle (0: all the renderer drew)
    float crop_top = 0;   // (PC view) the part of the screen's height left out at its top (0-1)
    float crop_bottom = 0;  // ... and at its bottom (looking up)
    Picture picture = Picture::Composite;
    Rect src;             // (PC view) only this part of the screen, in DS pixels (w 0: the whole screen)
    bool crisp = false;   // drawn with hard pixel edges (2D at a whole-number scale)
    bool block = false;   // takes no clicks, and those on it reach nothing under it (a modal's backdrop)
};

// What the One screen layout shows (see Presenter::set_one_mode).
enum class OneMode {
    Joined,  // both screens' 3D, stacked into one picture
    Flat,    // no 3D (menus): both 4:3, side by side
    Solo,    // 3D on one screen only: that one fills the window, the other is a picture-in-picture
};

class FlipPresenter;

class Presenter {
public:
    // The most views a layout shows (the PC view: two parts of each screen, and the top screen's 2D once more).
    static constexpr int kMaxViews = 12;

    Presenter();
    ~Presenter();

    // Creates the GL context (4.3 core if available, else 3.3 core) on `window` and loads its functions. On Windows,
    // frames are shown through a DXGI flip-model swapchain when the driver allows it (unless `allow_flip_model` is
    // false), else with an ordinary GL swap.
    bool init(SDL_Window* window, bool allow_flip_model = true);
    // Destroys the context (before the window goes).
    void shutdown();

    bool has_compute() const { return gl43_; }
    bool flip_model() const { return flip_ != nullptr; }

    // Widescreen: the top screen is (256 + 2 * extra) pixels wide (0 = 4:3), like the renderer's output texture, in
    // which the bottom screen is in the middle. Takes effect with the next layout().
    // With `both`, the bottom screen is as wide as the top (the game draws 3D on both).
    void set_wide(int extra, bool both = false) {
        wide_extra_ = extra;
        wide_both_ = both && extra > 0;
    }
    int wide_extra() const { return wide_extra_; }
    // One screen layout, while the game shows no widened 3D (its 2D menus): the screens are shown 4:3 (the middle of
    // the wide output), side by side, as large as fits. Takes effect with the next layout().
    // With Solo, `screen` is the one with 3D, and `pip` the size of the other (0 small, 1 large, 2 hidden). A change of
    // mode moves the screens to their new places over a moment rather than at once. Takes effect with the next layout().
    void set_one_mode(OneMode mode, int screen = 1, int pip = 0) {
        flat_ = mode == OneMode::Flat;
        one_mode_ = mode;
        solo_screen_ = screen;
        pip_ = pip;
    }
    OneMode one_mode() const { return one_mode_; }
    int solo_screen() const { return solo_screen_; }
    int pip() const { return pip_; }
    bool flat() const { return flat_; }
    // While the picture-in-picture is small or hidden, it is shown large for a moment (its screen changed: new text).
    void pip_attention(bool on) { pip_attention_ = on; }
    // PC view, joined: the top screen's 2D is drawn over its own part of the picture too, not only in its corner panel
    // (a fade: the game darkens both screens with a 2D layer in front of the 3D). Takes effect with the next layout().
    void set_cover_top(bool on) { cover_top_ = on; }
    // ... and how dark the fade makes it (0-1, the black layer's weight): the wide sides, which have no 2D, are darkened
    // as much, so the whole width fades together.
    void set_fade_dark(float dark) { fade_dark_ = dark; }
    bool cover_top() const { return cover_top_; }
    // PC view (docs/PC_VIEW.md), parts of the touch screen's 2D on places of their own, at a whole-number scale (the
    // window's height / 360: 2 at 720p, 3 at 1080p) and drawn crisp, clicks on them going to where they are on the DS
    // (and none to where they were). Each takes effect with the next layout().
    // The HUD (the bag at the top left, the hand at the top right of the touch screen) in the window's lower corners.
    void set_pc_hud(bool on) { pc_hud_ = on; }
    bool pc_hud() const { return pc_hud_; }
    // The text box: the touch screen's rows from `top` (DS pixels) down anchored to the bottom middle of the window
    // (with the choices above it); < 0: none.
    void set_pc_text(float top) { pc_text_top_ = top; }
    float pc_text() const { return pc_text_top_; }
    // A menu (the bag, the Start menu's pages, the map): the two screens side by side in the middle of the window over
    // the dimmed picture from before it opened, the top screen (information) left, the touch screen right.
    void set_pc_modal(bool on) { pc_modal_ = on; }
    bool pc_modal() const { return pc_modal_; }
    // How far down the touch screen's 2D reaches up from its bottom edge, in DS rows, from the probe of its 2D part
    // (the lowest row from which every row below has something drawn); -1 if not known. Measured with screen_peak.
    int bottom_overlay_top() const { return bottom_top_; }
    // ... and whether its bottom 16 rows are drawn across most of its width (a text box is there).
    bool bottom_overlay_box() const { return bottom_box_; }
    // ... and whether it is drawn over (nearly) all of it (a curtain while a scene is set up, a fade's black layer).
    bool bottom_overlay_cover() const { return bottom_cover_; }
    // Joined: how far the picture looks up (0 as laid out, about 70% ground; 1 the top screen whole, the sky).
    void set_look_up(float t) { look_up_ = t; }
    float look_up() const { return look_up_; }
    int pc_scale(int height) const { return std::max(1, height / 360); }
    // How much of each screen's picture is lit: how many of 16 x 12 points across its DS pixels are not (nearly) black
    // (0-192), measured every few frames while asked for, without waiting for the GPU (so a few frames late); -1 if
    // not known.
    void measure_screens(bool on) {
        if (!on) peak_[0] = peak_[1] = peak_[2] = bottom_top_ = -1;
        probe_on_ = on;
    }
    int screen_peak(int screen) const { return peak_[screen]; }
    // PC view: how much of the top screen's 2D part (its overlay, the layer split's) is drawn: how many of 128 x 96
    // points across its DS pixels are not see-through (0-12288), measured with screen_peak; -1 if not known.
    int top_overlay_peak() const { return peak_[2]; }
    // One screen, joined: a screen without 3D of its own is shown 4:3 in the middle of its place, over a dim stretched
    // copy of itself rather than black sides.
    void set_ambient(bool top, bool bottom) {
        ambient_[0] = top;
        ambient_[1] = bottom;
    }
    // Lays the screens out for the window's current size.
    void layout(int window_px_w, int window_px_h, const LayoutSettings& settings);

    // The Windows layout: the other screen is shown in a window of its own (opened and closed by the frontend, which
    // owns it), drawn and shown with every frame that present() shows. It has no menu or screenshots.
    bool open_second(SDL_Window* window, bool allow_flip_model = true);
    void close_second();
    bool has_second() const { return window2_ != nullptr; }
    void layout_second(int window_px_w, int window_px_h, const LayoutSettings& settings);
    // The screen under a position in the second window (see screen_at).
    int screen_at_second(float px, float py, float* local_x, float* local_y) const;
    // The visible screens in drawing order (later ones are on top).
    int view_count() const { return view_count_; }
    const ScreenView& view(int i) const { return views_[i]; }
    // The screen drawn at a window position (topmost), or -1. Sets `local` to the position on that screen in DS pixels.
    int screen_at(float px, float py, float* local_x, float* local_y) const;
    // The tallest screen on the window, in pixels (what the internal resolution should cover).
    float largest_screen_height() const;

    // Draws a frame (or black if `frame` is null) to the back buffer, and leaves it bound for drawing over.
    void draw(const ntr::Frame* frame, Filter filter);
    // Shows the back buffer. With `screenshot`, first saves it as a PNG.
    void swap(const char* screenshot = nullptr);
    // Display-rate presentation (vsync): swap() shows frames in step with the display's refresh, and
    // wait_for_refresh() returns when the display can take the next frame, so that the frame drawn after it is shown
    // at the following refresh. Off: frames show at once and the frontend paces them.
    void set_vsync(bool on);
    bool vsync() const { return vsync_; }
    // Without vsync, frames are shown at the moment they are presented, tearing allowed (variable refresh rate
    // displays). Needs a swapchain or driver that can; see tearing_supported().
    void set_tearing(bool on) { tearing_ = on; }
    bool tearing_supported() const;
    void wait_for_refresh();
    // The refresh rate of the display the window is on (Hz), or 0 if unknown.
    float refresh_rate() const;
    // A picture shown over part of the top screen, in place of what the game draws there (the Wii cutscenes): `rgb` is w * h * 3
    // bytes, and it covers the DS pixels `ds` of the 256 x 192 screen. It is uploaded when `serial` differs from the last
    // call's. The context must be current (the thread that presents).
    void set_picture(const std::uint8_t* rgb, int w, int h, std::uint64_t serial, const Rect& ds);
    void clear_picture() { picture_on_ = false; }
    // The gamepad's stylus: a pointer drawn over the touch screen at DS pixel (x, y), filled while it touches.
    void set_cursor(bool visible, float x, float y, bool pressed) {
        cursor_on_ = visible;
        cursor_x_ = x;
        cursor_y_ = y;
        cursor_pressed_ = pressed;
    }
    // Drawn over every frame that present() shows (the settings menu), before it is saved or swapped in.
    std::function<void()> overlay;
    void present(const ntr::Frame* frame, Filter filter, const char* screenshot = nullptr) {
        gpu_begin();
        draw(frame, filter);
        if (overlay) overlay();
        gpu_end();
        swap(screenshot);
        if (window2_) show_second(frame, filter, screenshot);
    }

    // Testing (--perf-log, --frame-hash). The hash of what was swapped in since the last call (every present adds to it);
    // it covers the whole window, so it only matches between runs with the same window size and settings.
    void hash_pictures(bool on) { hash_pictures_ = on; }
    std::uint64_t take_picture_hash() {
        const std::uint64_t h = picture_hash_;
        picture_hash_ = 0;
        return h;
    }
    // The GPU time of present()'s drawing, from timer queries read a few presents late so that nothing stalls. Each
    // present has a serial (1, 2, ...; present_serial() is the last one begun); poll_gpu_time() gives the finished ones in
    // order, and not at all where the driver has no timer queries.
    void gpu_timing(bool on) { gpu_timing_ = on; }
    std::uint64_t present_serial() const { return serial_; }
    bool poll_gpu_time(std::uint64_t& serial, double& ms);

private:
    SDL_Surface* read_back(unsigned target, int width, int height);  // the pixels of a framebuffer, top row first
    void save_png(const char* path, unsigned target, int width, int height);
    void gpu_begin();
    void gpu_end();
    void draw_views(unsigned target, int width, int height, const ScreenView* views, int count, const ntr::Frame* frame,
                    Filter filter);
    void show_second(const ntr::Frame* frame, Filter filter, const char* screenshot);
    void draw_picture(const Rect& r, float opacity, Filter filter, int width, int height, const ntr::Frame* frame);

    SDL_Window* window_ = nullptr;
    void* context_ = nullptr;  // SDL_GLContext
    std::unique_ptr<FlipPresenter> flip_;
    unsigned target_ = 0;      // the framebuffer this frame is drawn into (0 = the window's own)
    bool gl43_ = false;
    bool vsync_ = false;
    bool tearing_ = false;
    unsigned program_ = 0, vbo_ = 0, vao_ = 0, soft_tex_ = 0;
    unsigned picture_tex_ = 0;  // set_picture's picture (a one-layer array texture, as the screens' shader reads)
    int picture_w_ = 0, picture_h_ = 0;
    std::uint64_t picture_serial_ = ~std::uint64_t(0);
    bool picture_on_ = false;
    Rect picture_ds_;
    int wide_extra_ = 0;
    bool wide_both_ = false;
    bool flat_ = false;
    OneMode one_mode_ = OneMode::Joined, laid_mode_ = OneMode::Joined;
    int solo_screen_ = 1, pip_ = 0, laid_pip_ = 0, laid_solo_ = 1;
    bool laid_one_ = false;
    bool pip_attention_ = false, laid_attention_ = false;
    bool cover_top_ = false;
    float fade_dark_ = 0;
    // The move between two arrangements: the views as they were shown when it began, and when that was (ms).
    ScreenView anim_from_[kMaxViews];
    int anim_from_count_ = 0;
    std::uint64_t anim_start_ = 0;
    void solo_layout(int w, int h);
    void pc_view_layout(int w, int h);
    void probe(const ntr::Frame* frame);
    bool probe_on_ = false;
    int peak_[3] = {-1, -1, -1}, probe_count_ = 0, bottom_top_ = -1;
    bool probe_bottom_ = false;  // (the probe in flight measures the bottom 2D part too)
    bool bottom_box_ = false, bottom_cover_ = false;
    bool pc_hud_ = false, pc_modal_ = false, laid_modal_ = false;
    float pc_text_top_ = -1, look_up_ = 0;
    Rect holes_[4];  // (PC view) the touch screen's parts drawn elsewhere: left out where it is drawn whole
    int hole_count_ = 0;
    unsigned backdrop_tex_ = 0, backdrop_fbo_ = 0;  // (PC view) the last picture before a modal
    int backdrop_w_ = 0, backdrop_h_ = 0;
    bool backdrop_ok_ = false;
    // (three layers written in turn, 10 pictures each: the modal shows the one before the last, 10 to 20 pictures old,
    // so a picture the game drew as the menu opened, a frame without 3D, is never its backdrop)
    int backdrop_layer_ = 0, backdrop_count_ = 0, backdrop_full_ = 0;
    void save_backdrop();
    void pc_parts(int w, int h, const Rect& bottom);
    void pc_modal_layout(int w, int h);
    int u_holes_ = -1, u_hole_count_ = -1;
    bool pc_view_ = false;  // (laid out for the PC view)
    bool probe_overlay_ = false;  // (the probe in flight measures the top 2D part too)
    bool ambient_[2] = {false, false};
    unsigned probe_fbo_[2] = {0, 0}, probe_tex_ = 0, probe_pbo_ = 0;
    void* probe_fence_ = nullptr;  // GLsync
    void layout_views(int w, int h, const LayoutSettings& s);
    int current_views(ScreenView* out) const;  // the views as shown now (moving, or views_)
    bool cursor_on_ = false, cursor_pressed_ = false;
    float cursor_x_ = 0, cursor_y_ = 0;
    void draw_cursor(const ScreenView& v, int height);
    // The width of a screen in DS pixels as it is shown (256, or wider with widescreen).
    float shown_width(int screen) const {
        return !flat_ && (screen == 0 || wide_both_) ? float(256 + 2 * wide_extra_) : 256.0f;
    }
    float shown_width(const ScreenView& v) const { return v.shown > 0 ? v.shown : shown_width(v.screen); }
    int u_uv_x_ = -1, u_uv_y_ = -1;
    int u_dest_ = -1, u_layer_ = -1, u_sharp_ = -1, u_out_size_ = -1, u_tex_ = -1, u_opacity_ = -1, u_alpha_ = -1;
    int width_ = 0, height_ = 0;
    ScreenView views_[kMaxViews];
    int view_count_ = 0;
    SDL_Window* window2_ = nullptr;  // the second window (Windows layout)
    std::unique_ptr<FlipPresenter> flip2_;
    int width2_ = 0, height2_ = 0;
    ScreenView view2_;
    bool hash_pictures_ = false, gpu_timing_ = false, gpu_active_ = false;
    std::uint64_t picture_hash_ = 0, serial_ = 0;
    struct GpuQuery {
        unsigned id = 0;
        std::uint64_t serial = 0;
    };
    std::deque<GpuQuery> gpu_pending_;  // begun and ended, not yet read
    std::vector<unsigned> gpu_free_;    // query objects not in use
};

}  // namespace video
