// Player settings: what the settings menu edits, saved as `key = value` lines in settings.ini next to the save.
#pragma once

#include <filesystem>

#include "cfie_game.hpp"
#include "input.hpp"
#include "ntr/runtime.hpp"
#include "video.hpp"

// melonDS's compute-shader renderer is switched off for now: widescreen, interpolation, Stable textures and texture
// smoothing are built on the OpenGL renderer, and none of them is done for compute. The code stays; set this to true to
// bring it back (a stored or requested "compute" becomes OpenGL while it is false).
inline constexpr bool kComputeRenderer = false;

// The top screen's aspect ratio (OpenGL renderer). The DS's 256 x 192 picture grows by "extra" 3D pixels on each side.
enum class Aspect { Original, W16x9, W16x10, W21x9, W32x9 };
inline int aspect_extra(Aspect a) {
    constexpr int kExtra[] = {0, 43, 26, 100, 214};  // (192 * ratio - 256) / 2: 16:9, 16:10, 64:27, 32:9
    return kExtra[int(a)];
}

// How frames are timed with 3D at the display's refresh rate: shown in step with the display (vsync), or presented on a
// timer of the frontend's own, at once, for a variable refresh rate display (G-Sync, FreeSync).
enum class Pacing { Vsync, Variable };

// The language the game plays in: the DS firmware's language setting, which the game reads at start. The USA release
// has English, French and Spanish; the European adds German (a language a release lacks plays in English).
enum class GameLanguage { English, French, German, Spanish };

struct Settings {
    // Display
    video::LayoutSettings layout;
    bool fullscreen = false;
    Aspect aspect = Aspect::Original;  // a wider top screen (OpenGL renderer)
    bool texture_pack = true;  // OpenGL: draw replacements from <data>/textures/load (texture_pack.hpp)
    bool texture_dump = false;
    bool texture_upscale = false;  // OpenGL: textures enlarged 4x with Scale2x twice (smoothed diagonals, crisp edges)  // OpenGL: save each new texture to <data>/textures/dump
    bool pause_unfocused = true;  // the game waits while its window is not focused
    int walk_speed = 100;     // Magician's Quest: the game's speed while you walk (percent: 100, 150, 200)
    bool supersample = false;  // OpenGL, automatic resolution: twice the window's, averaged down (anti-aliasing)
    int view_zoom = 0;  // Magician's Quest, widescreen: the 3D magnified, percent (0: just enough to fill the width)
    bool world_extend = false;  // OpenGL widescreen: the ground drawn on into the sides the game never built
    bool smooth_2d = true;    // OpenGL: text, menus and sprites upscaled smoothly (rounded diagonals) rather than as blocks
    bool fill_edges = true;   // widescreen: empty parts of the sides continue the scene beside them, darkened
    bool hud_edges = false;   // wider than 4:3: the HUD at the edges of the picture rather than the middle
    int hud_scale = 100;      // the HUD's size in percent (50-200)
    // Graphics
    ntr::Renderer renderer = ntr::Renderer::OpenGL;
    int scale = 0;  // internal resolution multiplier, 0 = match the window
    video::Filter filter = video::Filter::Linear;
    bool better_polygons = false;
    bool hires_coordinates = true;
    ntr::TextureSmoothing texture_smoothing = ntr::TextureSmoothing::Off;  // blend the game's textures (OpenGL renderer)
    bool stable_textures = false;  // cut quads with warping texture coordinates into a grid (see ntr::VideoSettings)
    bool interpolate = false;   // 3D at 60 fps (frame interpolation; off: Magician's Quest alternates its 3D between the screens)
    bool display_rate = false;  // with interpolate: 3D at the display's refresh rate (OpenGL/compute renderers)
    Pacing pacing = Pacing::Vsync;  // with display_rate
    int rate_cap = 0;           // with Pacing::Variable: frames per second to present (0 = the display's refresh rate)
    // Audio
    int volume = 100;  // percent
    bool menu_sounds = true;  // the game's own sounds for the menus
    // Interface
    int ui_scale = 0;           // the menus' size in percent (50-200), 0 = the display's
    bool skip_prelaunch = false;  // start straight into the game when it is set up
    bool check_updates = true;    // look for a newer release on GitHub at start (see update_check.hpp)
    bool preset_chosen = false;   // the Welcome window has been answered (it shows on the first run)
    bool suppress_preset = false;  // not saved: a test run offers no Welcome window (and leaves preset_chosen alone)
    // Game
    GameLanguage language = GameLanguage::English;
    bool skip_logos = true;
    bool wii_cutscenes = true;  // cutscenes from the Wii version's files, when they are set up (see wii_cutscenes.hpp)
    bool fast_loading = true;   // cartridge reads without the card's delays
    bool fast_saving = true;   // save reads and writes at once instead of at the DS's pace
    // Cheats: the game's clock this far ahead of the PC's (behind when negative): time travel, as with a DS's clock.
    int clock_days = 0, clock_hours = 0;
    cfie::TextSpeed text_speed = cfie::TextSpeed::Fast;
    bool quick_transitions = true;  // fades between places run at fast-forward speed  // dialogue text: the DS's pace, three times as fast, or at once
    int cpu_speed = 1;         // ARM9 clock: 1, 2 or 4 times the DS's (the runtime boots at 1 either way)
    // Online
    std::string console_name;  // the console's name other players see (a DS nickname, 10 characters); empty: this PC's
    std::string online_server = "kaeru";  // a Wi-Fi Connection revival server (see online_dns), or a DNS address
    cfie::OnlineBuffer online_buffer = cfie::OnlineBuffer::Low;
    // Co-op between PCs (the game's Multi-Card Play over the network): off, host, or join the host at lan_address.
    int lan_mode = 0;  // 0 off, 1 host, 2 join
    std::string lan_address;
    int lan_port = 49251;
    // ... over the internet (docs/COOP_INTERNET.md): the game's own exchange stays on each PC, with a hidden stand-in
    // copy of the game for the other player; only game data crosses the network. For round trips over ~20 ms.
    bool coop_internet = false;
    // Co-op: connect at once (game/src/coop_connect.cpp) as soon as the student is free to move after loading:
    // 0 off, 1 as host, 2 as guest (visiting the host's town).
    int coop_auto = 0;
    // Co-op: the town on the top screen in a visit too (3D on both screens, as alone), instead of the DS's pattern.
    bool coop_top_3d = true;
    // Controls
    input::Bindings controls = input::Bindings::defaults();
};

// Reads the settings that are present in `path` into `s` (unknown keys and bad values are ignored).
// Returns false if the file does not exist.
bool load_settings(const std::filesystem::path& path, Settings& s);
bool save_settings(const std::filesystem::path& path, const Settings& s);
// Sets one setting by its settings.ini key and value ("display.aspect", "21:9"). False if either is not valid.
bool set_setting(Settings& s, const std::string& key, const std::string& value);

// A console name as the DS can show it: Latin characters only, at most 10 of them (UTF-8). Empty if nothing is left.
std::string sanitize_console_name(const std::string& name);
// The console name the game gets: the setting, or "Wizard" if it is empty (other players see it: never the PC's name).
std::string resolved_console_name(const Settings& s);
// The Welcome presets are still to be answered (and this run is not a test run).
inline bool preset_pending(const Settings& s) { return !s.preset_chosen && !s.suppress_preset; }

// The DNS server for an online server setting: "wiilink" (WiiLink WFC), "kaeru" (Kaeru WFC, on Wiimmfi), "host" (the
// host's own DNS, 0.0.0.0) or a DNS address such as "5.161.56.11". False if it is none of these.
bool online_dns(const std::string& server, std::array<uint8_t, 4>& dns);
